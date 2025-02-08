// clang-format off
#include "../../r_backend.h"
#include "sokol_local.h"
#include "sokol_imgui.h"
#include "sokol_gfx_imgui.h"
#include "sokol_pipeline.h"
#include "sokol_images.h"

#ifdef SOKOL_D3D11
#include "sokol_d3d11.h"
#endif

#include "epi.h"
#include "i_video.h"

// clang-format on

extern ConsoleVariable vsync;
void                   BSPStartThread();
void                   BSPStopThread();
void                   RenderStartThread();
void                   RenderStopThread();
void RenderQueueDrawContext(sgl_context context);

constexpr int32_t kWorldStateInvalid     = -1;
constexpr int32_t kRenderContextPoolSize = 256;

constexpr int32_t kRenderLayerSky_MaxCommands = 32 * 1024;
constexpr int32_t kRenderLayerSky_MaxVertices = 128 * 1024;

constexpr int32_t kRenderLayerSolid_MaxCommands = 8 * 1024;
constexpr int32_t kRenderLayerSolid_MaxVertices = 16 * 1024;

constexpr int32_t kRenderLayerTransparent_MaxCommands = 128 * 1024;
constexpr int32_t kRenderLayerTransparent_MaxVertices = 256 * 1024;

constexpr int32_t kRenderLayerHUD_MaxCommands = 32 * 1024;
constexpr int32_t kRenderLayerHUD_MaxVertices = 128 * 1024;

constexpr int32_t kRenderLayerWeapon_MaxCommands = 32 * 1024;
constexpr int32_t kRenderLayerWeapon_MaxVertices = 128 * 1024;

class SokolRenderBackend : public RenderBackend
{
  public:
    void SetupMatrices2D()
    {
        sgl_viewport(0, 0, current_screen_width, current_screen_height, false);

        sgl_matrix_mode_projection();
        sgl_load_identity();
        sgl_ortho(0.0f, (float)current_screen_width, 0.0f, (float)current_screen_height, -1.0f, 1.0f);

        sgl_matrix_mode_modelview();
        sgl_load_identity();
    }

    void SetupWorldMatrices2D()
    {
        sgl_viewport(view_window_x, view_window_y, view_window_width, view_window_height, false);

        sgl_matrix_mode_projection();
        sgl_load_identity();
        sgl_ortho((float)view_window_x, (float)view_window_width, (float)view_window_y, (float)view_window_height,
                  -1.0f, 1.0f);

        sgl_matrix_mode_modelview();
        sgl_load_identity();
    }

    void SetupMatrices3D()
    {
        sgl_viewport(view_window_x, view_window_y, view_window_width, view_window_height, false);

        // calculate perspective matrix

        sgl_matrix_mode_projection();
        sgl_load_identity();

        sgl_frustum(-view_x_slope * renderer_near_clip.f_, view_x_slope * renderer_near_clip.f_,
                    -view_y_slope * renderer_near_clip.f_, view_y_slope * renderer_near_clip.f_, renderer_near_clip.f_,
                    renderer_far_clip.f_);

        // calculate look-at matrix
        sgl_matrix_mode_modelview();
        sgl_load_identity();
        sgl_rotate(sgl_rad(270.0f) - epi::RadiansFromBAM(view_vertical_angle), 1.0f, 0.0f, 0.0f);
        sgl_rotate(sgl_rad(90.0f) - epi::RadiansFromBAM(view_angle), 0.0f, 0.0f, 1.0f);
        sgl_translate(-view_x, -view_y, -view_z);
    }

    void StartFrame(int32_t width, int32_t height)
    {
        frame_number_++;

#ifdef SOKOL_D3D11
        if (deferred_resize)
        {
            deferred_resize = false;
            sapp_d3d11_resize_default_render_target(deferred_resize_width, deferred_resize_height);
        }
#endif

        FinalizeDeletedImages();

        sg_pass_action pass_action;
        pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass_action.colors[0].clear_value = {epi::GetRGBARed(clear_color_) / 255.0f,
                                             epi::GetRGBAGreen(clear_color_) / 255.0f,
                                             epi::GetRGBABlue(clear_color_) / 255.0f, 1.0f};

        pass_action.depth.load_action = SG_LOADACTION_CLEAR;
        pass_action.depth.clear_value = 1.0f;
        pass_action.stencil           = {SG_LOADACTION_CLEAR, SG_STOREACTION_DONTCARE, 0};

        EPI_CLEAR_MEMORY(&pass_, sg_pass, 1);
        pass_.action                   = pass_action;
        pass_.swapchain.width          = width;
        pass_.swapchain.height         = height;
        pass_.swapchain.color_format   = SG_PIXELFORMAT_RGBA8;
        pass_.swapchain.depth_format   = SG_PIXELFORMAT_DEPTH;
        pass_.swapchain.gl.framebuffer = 0;

#ifdef SOKOL_D3D11
        pass_.swapchain.d3d11.render_view        = sapp_d3d11_get_render_view();
        pass_.swapchain.d3d11.resolve_view       = sapp_d3d11_get_resolve_view();
        pass_.swapchain.d3d11.depth_stencil_view = sapp_d3d11_get_depth_stencil_view();
#endif

        imgui_frame_desc_            = {0};
        imgui_frame_desc_.width      = width;
        imgui_frame_desc_.height     = height;
        imgui_frame_desc_.delta_time = 100;
        imgui_frame_desc_.dpi_scale  = 1;

        for (int32_t i = 0; i < kRenderWorldMax; i++)
        {
            world_state_[i].active_ = false;
            world_state_[i].used_   = false;
            for (int32_t j = 0; j < kRenderLayerMax; j++)
            {
                world_state_[i].layers_[j].context_ = nullptr;
            }
        }

        EPI_CLEAR_MEMORY(&render_state_, RenderState, 1);
        render_state_.world_state_ = kWorldStateInvalid;

        EPI_CLEAR_MEMORY(global_layers_, RenderLayer, kRenderLayerMax);

        for (int32_t i = 0; i < kRenderContextPoolSize; i++)
        {
            context_pool_[i].active_ = false;
            context_pool_[i].next_   = nullptr;
        }

        SetRenderLayer(kRenderLayerHUD);

        sg_begin_pass(&pass_);
    }

    void SwapBuffers()
    {
#ifdef SOKOL_D3D11
        sapp_d3d11_present(vsync.d_ ? false : true, vsync.d_ ? 1 : 0);
#endif
    }

    void FinishFrame()
    {
        EDGE_ZoneNamedN(ZoneFinishFrame, "BackendFinishFrame", true);

        {
            EDGE_ZoneNamedN(ZoneDrawWorlds, "DrawWorlds", true);
            // World
            for (int32_t i = 0; i < kRenderWorldMax; i++)
            {
                const WorldState *state = &world_state_[i];

                if (!state->used_)
                {
                    break;
                }

                for (int32_t j = 0; j < kRenderLayerMax; j++)
                {
                    RenderContext *context = state->layers_[j].context_;
                    while (context)
                    {
                        sgl_context_draw(context->sgl_context_);
                        // TODO: this reverses draw order
                        context = context->next_;
                    }
                }
            }
        }

        {
            EDGE_ZoneNamedN(ZoneDrawHud, "DrawHud", true);
            // Hud
            sgl_context_draw(context_pool_[0].sgl_context_);
        }

        {
            EDGE_ZoneNamedN(ZoneDrawDefaultLayer, "DrawDefaultLayer", true);
            // default layer
            // sgl_draw_layer(0);
        }

        {
            EDGE_ZoneNamedN(ZoneDrawImGui, "DrawImGui", true);
            sg_imgui_.caps_window.open        = false;
            sg_imgui_.buffer_window.open      = false;
            sg_imgui_.pipeline_window.open    = false;
            sg_imgui_.attachments_window.open = false;
            sg_imgui_.frame_stats_window.open = false;

            simgui_new_frame(&imgui_frame_desc_);
            sgimgui_draw(&sg_imgui_);

            simgui_render();
        }

        {
            EDGE_ZoneNamedN(ZoneDrawEndPass, "DrawEndPass", true);
            sg_end_pass();
        }

        {
            EDGE_ZoneNamedN(ZoneDrawCommit, "DrawCommit", true);
            sg_commit();
        }

        for (auto itr = on_frame_finished_.begin(); itr != on_frame_finished_.end(); itr++)
        {
            (*itr)();
        }

        on_frame_finished_.clear();
    }

    void Flush(int32_t commands, int32_t vertices)
    {
        if (!render_state_.render_layer_ || render_state_.render_layer_->type_ != kRenderLayerSolid ||
            !render_state_.render_layer_->context_)
        {
            return;
        }

        int num_commands = sgl_num_commands();
        int num_vertices = sgl_num_vertices();

        if ((num_vertices + vertices) >= kRenderLayerSolid_MaxVertices ||
            (num_commands + commands) >= kRenderLayerSolid_MaxCommands)
        {
            RenderContext *current = render_state_.render_layer_->context_;
            if (current)
            {
                RenderQueueDrawContext(current->sgl_context_);
            }
            EPI_ASSERT(current->active_);
            render_state_.render_layer_->context_ = nullptr;

            for (int32_t i = 0; i < kRenderContextPoolSize; i++)
            {
                if (context_pool_[i].layer_type_ == kRenderLayerSolid && !context_pool_[i].active_ &&
                    context_pool_[i].max_commands_)
                {

                    context_pool_[i].active_                     = true;
                    render_state_.render_layer_->context_        = &context_pool_[i];
                    //render_state_.render_layer_->context_->next_ = current;                    

                    EPI_ASSERT(render_state_.render_layer_->context_ != current);
                    break;
                }
            }

            EPI_ASSERT(render_state_.render_layer_->context_ && render_state_.render_layer_->context_->active_);

            sgl_set_context(render_state_.render_layer_->context_->sgl_context_);
            SetupMatrices3D();
        }
    }

    void Resize(int32_t width, int32_t height)
    {
#ifdef SOKOL_D3D11
        deferred_resize        = true;
        deferred_resize_width  = width;
        deferred_resize_height = height;
#else
        EPI_UNUSED(width);
        EPI_UNUSED(height);
#endif
    }

    void Shutdown()
    {
#ifdef SOKOL_D3D11
        sapp_d3d11_destroy_device_and_swapchain();
#endif

        RenderStopThread();
        BSPStopThread();
    }

#if defined(SOKOL_GLCORE) || defined(SOKOL_GLES3)
    void CaptureScreenGL(int32_t width, int32_t height, int32_t stride, uint8_t *dest)
    {
        for (int32_t y = 0; y < height; y++)
        {
            render_state->ReadPixels(0, y, width, 1, GL_RGBA, GL_UNSIGNED_BYTE, dest);
            dest += stride;
        }
    }
#endif

    void CaptureScreen(int32_t width, int32_t height, int32_t stride, uint8_t *dest)
    {
#if defined(SOKOL_GLCORE) || defined(SOKOL_GLES3)
        CaptureScreenGL(width, height, stride, dest);
#endif

#ifdef SOKOL_D3D11
        sapp_d3d11_capture_screen(width, height, stride, dest);
#endif
    }

    void Init()
    {
#if SOKOL_GLES3
        LogPrint("Sokol GLES3: Initialising...\n");
#elif SOKOL_GLCORE
        LogPrint("Sokol GL: Initialising...\n");
#else
        LogPrint("Sokol D3D11: Initialising...\n");
#endif

        // TODO: should be able to query from sokol?
        max_texture_size_ = 4096;

        sg_environment env;
        EPI_CLEAR_MEMORY(&env, sg_environment, 1);
        env.defaults.color_format = SG_PIXELFORMAT_RGBA8;
        env.defaults.depth_format = SG_PIXELFORMAT_DEPTH;
        env.defaults.sample_count = 1;

#ifdef SOKOL_D3D11
        sapp_d3d11_init(program_window, current_screen_width, current_screen_height);
        env.d3d11.device         = sapp_d3d11_get_device();
        env.d3d11.device_context = sapp_d3d11_get_device_context();
#endif

        sg_desc desc;
        EPI_CLEAR_MEMORY(&desc, sg_desc, 1);
        desc.environment        = env;
        desc.logger.func        = slog_func;
        desc.pipeline_pool_size = 512 * 8;
        desc.buffer_pool_size   = 512;
        desc.image_pool_size    = 8192;

        sg_setup(&desc);

        if (!sg_isvalid())
        {
            FatalError("Sokol invalid");
        }

        sgl_desc_t sgl_desc;
        EPI_CLEAR_MEMORY(&sgl_desc, sgl_desc_t, 1);
        sgl_desc.color_format       = SG_PIXELFORMAT_RGBA8;
        sgl_desc.depth_format       = SG_PIXELFORMAT_DEPTH;
        sgl_desc.sample_count       = 1;
        sgl_desc.pipeline_pool_size = 512 * 8;
        sgl_desc.logger.func        = slog_func;
        sgl_desc.context_pool_size  = kRenderContextPoolSize;
        sgl_setup(&sgl_desc);

        // Base context
        sgl_context_desc_t context_desc_2d;
        EPI_CLEAR_MEMORY(&context_desc_2d, sgl_context_desc_t, 1);
        context_desc_2d.color_format = SG_PIXELFORMAT_RGBA8;
        context_desc_2d.depth_format = SG_PIXELFORMAT_DEPTH;
        context_desc_2d.sample_count = 1;

        EPI_CLEAR_MEMORY(context_pool_, RenderContext, kRenderContextPoolSize);

        int32_t current_context = 0;

        // HUD
        int32_t max_commands         = kRenderLayerHUD_MaxCommands;
        int32_t max_vertices         = kRenderLayerHUD_MaxVertices;
        context_desc_2d.max_commands = max_commands;
        context_desc_2d.max_vertices = max_vertices;

        context_pool_[current_context].layer_type_   = kRenderLayerHUD;
        context_pool_[current_context].max_commands_ = max_commands;
        context_pool_[current_context].max_vertices_ = max_vertices;
        context_pool_[current_context].sgl_context_  = sgl_make_context(&context_desc_2d);
        current_context++;

        // Player Weapon
        max_commands                 = kRenderLayerWeapon_MaxCommands;
        max_vertices                 = kRenderLayerWeapon_MaxVertices;
        context_desc_2d.max_commands = max_commands;
        context_desc_2d.max_vertices = max_vertices;

        context_pool_[current_context].layer_type_   = kRenderLayerWeapon;
        context_pool_[current_context].max_commands_ = max_commands;
        context_pool_[current_context].max_vertices_ = max_vertices;
        context_pool_[current_context].sgl_context_  = sgl_make_context(&context_desc_2d);
        current_context++;

        for (int32_t i = 0; i < kRenderWorldMax; ++i)
        {
            // Solid batches
            max_commands                 = kRenderLayerSolid_MaxCommands;
            max_vertices                 = kRenderLayerSolid_MaxVertices;
            context_desc_2d.max_commands = max_commands;
            context_desc_2d.max_vertices = max_vertices;

            for (int32_t j = 0; j < 16; j++)
            {
                context_pool_[current_context].layer_type_   = kRenderLayerSolid;
                context_pool_[current_context].max_commands_ = max_commands;
                context_pool_[current_context].max_vertices_ = max_vertices;
                context_pool_[current_context].sgl_context_  = sgl_make_context(&context_desc_2d);
                current_context++;
            }

            // Transparent
            max_commands                 = kRenderLayerTransparent_MaxCommands;
            max_vertices                 = kRenderLayerTransparent_MaxVertices;
            context_desc_2d.max_commands = max_commands;
            context_desc_2d.max_vertices = max_vertices;

            context_pool_[current_context].layer_type_   = kRenderLayerTransparent;
            context_pool_[current_context].max_commands_ = max_commands;
            context_pool_[current_context].max_vertices_ = max_vertices;
            context_pool_[current_context].sgl_context_  = sgl_make_context(&context_desc_2d);
            current_context++;

            // Sky
            max_commands                 = kRenderLayerSky_MaxCommands;
            max_vertices                 = kRenderLayerSky_MaxVertices;
            context_desc_2d.max_commands = max_commands;
            context_desc_2d.max_vertices = max_vertices;

            context_pool_[current_context].layer_type_   = kRenderLayerSky;
            context_pool_[current_context].max_commands_ = max_commands;
            context_pool_[current_context].max_vertices_ = max_vertices;
            context_pool_[current_context].sgl_context_  = sgl_make_context(&context_desc_2d);
            current_context++;
        }

        // IMGUI
        simgui_desc_t imgui_desc = {0};
        imgui_desc.logger.func   = slog_func;
        simgui_setup(&imgui_desc);

        const sgimgui_desc_t sg_imgui_desc = {0};
        sgimgui_init(&sg_imgui_, &sg_imgui_desc);

        InitPipelines();
        InitImages();

        EPI_CLEAR_MEMORY(world_state_, WorldState, kRenderWorldMax);
        for (int32_t i = 0; i < kRenderWorldMax; i++)
        {
            for (int32_t j = 0; j < kRenderLayerMax; j++)
            {
                world_state_[i].layers_[j].type_ = (RenderLayerType)j;
            }
        }

        EPI_CLEAR_MEMORY(&render_state_, RenderState, 1);
        render_state_.world_state_ = kWorldStateInvalid;

        RenderBackend::Init();

        BSPStartThread();
        RenderStartThread();
    }

    // FIXME: go away!
    void GetPassInfo(PassInfo &info)
    {
        info.width_  = pass_.swapchain.width;
        info.height_ = pass_.swapchain.height;
    }

    void SetClearColor(RGBAColor color)
    {
        clear_color_ = color;
    }

    int32_t GetHUDLayer()
    {
        return kRenderLayerHUD;
    }

    virtual void SetRenderLayer(RenderLayerType layer, bool clear_depth = false)
    {
        if (layer == kRenderLayerHUD || layer == kRenderLayerWeapon || render_state_.world_state_ == kWorldStateInvalid)
        {
            render_state_.render_layer_ = &global_layers_[layer];
        }
        else
        {
            render_state_.render_layer_ = &world_state_[render_state_.world_state_].layers_[layer];
        }

        if (!render_state_.render_layer_->context_)
        {
            for (int32_t i = 0; i < kRenderContextPoolSize; i++)
            {
                if (context_pool_[i].layer_type_ == layer && !context_pool_[i].active_ &&
                    context_pool_[i].max_commands_)
                {
                    context_pool_[i].active_              = true;
                    render_state_.render_layer_->context_ = &context_pool_[i];
                    break;
                }
            }

            EPI_ASSERT(render_state_.render_layer_->context_ && render_state_.render_layer_->context_->active_);
        }
        else
        {

            EPI_ASSERT(render_state_.render_layer_->context_->active_);
        }

        sgl_set_context(render_state_.render_layer_->context_->sgl_context_);

        if (clear_depth)
        {
            sgl_clear_depth(1.0f);
        }
    }

    RenderLayerType GetRenderLayer()
    {
        // TODO: check that we're not in a frame
        if (!render_state_.render_layer_)
        {
            sgl_set_context(context_pool_[0].sgl_context_);
            return kRenderLayerHUD;
        }

        return render_state_.render_layer_->type_;
    }

    void BeginWorldRender()
    {
        int32_t i = 0;
        for (; i < kRenderWorldMax; i++)
        {
            if (world_state_[i].active_)
            {
                FatalError("SokolRenderBackend: BeginWorldState called with active world");
            }

            if (!world_state_[i].used_)
            {
                break;
            }
        }

        if (i == kRenderWorldMax)
        {
            FatalError("SokolRenderBackend: BeginWorldState max worlds exceeded");
        }

        world_state_[i].active_    = true;
        world_state_[i].used_      = true;
        render_state_.world_state_ = i;
    }

    void FinishWorldRender()
    {
        render_state_.world_state_ = kWorldStateInvalid;

        int32_t i = 0;
        for (; i < kRenderWorldMax; i++)
        {
            if (world_state_[i].active_)
            {
                world_state_[i].active_ = false;
                break;
            }
        }

        if (i == kRenderWorldMax)
        {
            FatalError("SokolRenderBackend: FinishWorldState called with no active world render");
        }

        SetRenderLayer(kRenderLayerHUD);
        SetupMatrices2D();
    }

  private:
    struct RenderContext
    {
        RenderLayerType layer_type_;
        sgl_context     sgl_context_;
        int32_t         max_commands_;
        int32_t         max_vertices_;
        bool            active_;
        RenderContext  *next_;
    };

    struct RenderLayer
    {
        RenderLayerType type_;
        RenderContext  *context_;
    };

    struct RenderState
    {
        RenderLayer *render_layer_;
        int32_t      world_state_;
    };

    struct WorldState
    {
        bool        active_;
        bool        used_;
        RenderLayer layers_[kRenderLayerMax];
    };

    RenderLayer global_layers_[kRenderLayerMax];

    simgui_frame_desc_t imgui_frame_desc_;
    sgimgui_t           sg_imgui_;

    RGBAColor clear_color_ = kRGBABlack;

    RenderState render_state_;

    sg_pass pass_;

    WorldState world_state_[kRenderWorldMax];

    RenderContext context_pool_[kRenderContextPoolSize];

#ifdef SOKOL_D3D11
    bool    deferred_resize        = false;
    int32_t deferred_resize_width  = 0;
    int32_t deferred_resize_height = 0;
#endif
};

static SokolRenderBackend sokol_render_backend;
RenderBackend            *render_backend = &sokol_render_backend;
