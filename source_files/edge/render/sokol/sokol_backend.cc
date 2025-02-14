// clang-format off
#include "../../r_backend.h"
#include "sokol_local.h"
#include "sokol_pipeline.h"
#include "sokol_images.h"

#ifdef SOKOL_D3D11
#include "sokol_d3d11.h"
#endif

#include "epi.h"
#include "i_video.h"

#include "shaders/shader_screen.h"

// clang-format on

extern ConsoleVariable vsync;
extern ConsoleVariable debug_fps;

void BSPStartThread();
void BSPStopThread();

// from r_render.cc
void RendererEndFrame();

// from sokol_sky.cc
void SetupSkyMatrices(void);

constexpr int32_t kWorldStateInvalid = -1;

constexpr int32_t kContextPoolSize   = 32;
constexpr int32_t kContextMaxVertex  = 32 * 1024;
constexpr int32_t kContextMaxCommand = 2 * 1024;

EDGE_DEFINE_CONSOLE_VARIABLE(r_ssao, "1", kConsoleVariableFlagArchive)

class SokolRenderBackend : public RenderBackend
{
    struct WorldRender;

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

        if (debug_fps.d_ >= 3)
        {
            if (!sg_frame_stats_enabled())
            {
                sg_enable_frame_stats();
            }
        }
        else
        {
            if (sg_frame_stats_enabled())
            {
                sg_disable_frame_stats();
            }
        }

#ifdef SOKOL_D3D11
        if (deferred_resize)
        {
            deferred_resize = false;
            sapp_d3d11_resize_default_render_target(deferred_resize_width, deferred_resize_height);
        }
#endif

        FinalizeDeletedImages();

        render_state->Reset();

        EPI_CLEAR_MEMORY(world_state_, WorldState, kRenderWorldMax);

        EPI_CLEAR_MEMORY(&render_state_, RenderState, 1);
        render_state_.world_state_ = kWorldStateInvalid;

        SetRenderLayer(kRenderLayerHUD);
    }

    void Flush(int32_t commands, int32_t vertices)
    {
        if (commands >= kContextMaxCommand)
        {
            FatalError("RenderBackend: Flush called with commands that exceed context limit");
        }

        if (vertices >= kContextMaxVertex)
        {
            FatalError("RenderBackend: Flush called with vertices that exceed context limit");
        }

        int num_commands = sgl_num_commands();
        int num_vertices = sgl_num_vertices();

        if ((num_vertices + vertices) >= kContextMaxVertex || (num_commands + commands) >= kContextMaxCommand)
        {
            FlushContext();
        }
    }

    void SwapBuffers()
    {
#ifdef SOKOL_D3D11
        sapp_d3d11_present(vsync.d_ ? false : true, vsync.d_ ? 1 : 0);
#endif
    }

    void RenderWorldToScreen(WorldRender *render)
    {
        sg_apply_pipeline(screen_pipeline_);

        sg_bindings bind = {0};

        bind.vertex_buffers[0] = quad_buffer_;
        bind.images[0]         = render->color_target_;
        bind.samplers[0]       = render->color_sampler_;

        sg_apply_bindings(&bind);

        sg_draw(0, 6, 1);
    }

    void RenderHud()
    {
        sg_begin_pass(&hud_pass_);

        for (int32_t i = 0; i < kRenderWorldMax; i++)
        {
            if (world_state_[i].used_)
            {
                RenderWorldToScreen(&world_render_[i]);
            }
        }

        sgl_set_context(hud_context_);
        if (sgl_num_vertices())
        {
            sgl_context_draw(hud_context_);
        }
        sg_end_pass();
    }

    void FinishFrame()
    {
        EDGE_ZoneNamedN(ZoneFinishFrame, "BackendFinishFrame", true);

        RendererEndFrame();

        // HUD
        RenderHud();

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

        EPI_CLEAR_MEMORY(world_render_, WorldRender, kRenderWorldMax);

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
        sgl_desc.color_format = SG_PIXELFORMAT_RGBA8;
        sgl_desc.depth_format = SG_PIXELFORMAT_DEPTH;
        sgl_desc.sample_count = 1;
        // +1 default, +1 HUD + World Pools
        sgl_desc.context_pool_size  = kContextPoolSize * kRenderWorldMax + 2;
        sgl_desc.pipeline_pool_size = 512 * 8;
        sgl_desc.logger.func        = slog_func;
        sgl_setup(&sgl_desc);

        // HUD
        sg_pass_action pass_action;
        pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass_action.colors[0].clear_value = {epi::GetRGBARed(clear_color_) / 255.0f,
                                             epi::GetRGBAGreen(clear_color_) / 255.0f,
                                             epi::GetRGBABlue(clear_color_) / 255.0f, 1.0f};

        pass_action.depth.load_action = SG_LOADACTION_CLEAR;
        pass_action.depth.clear_value = 1.0f;
        pass_action.stencil           = {SG_LOADACTION_CLEAR, SG_STOREACTION_DONTCARE, 0};

        EPI_CLEAR_MEMORY(&hud_pass_, sg_pass, 1);
        hud_pass_.action                   = pass_action;
        hud_pass_.swapchain.width          = current_screen_width;
        hud_pass_.swapchain.height         = current_screen_height;
        hud_pass_.swapchain.color_format   = SG_PIXELFORMAT_RGBA8;
        hud_pass_.swapchain.depth_format   = SG_PIXELFORMAT_DEPTH;
        hud_pass_.swapchain.gl.framebuffer = 0;

#ifdef SOKOL_D3D11
        hud_pass_.swapchain.d3d11.render_view        = sapp_d3d11_get_render_view();
        hud_pass_.swapchain.d3d11.resolve_view       = sapp_d3d11_get_resolve_view();
        hud_pass_.swapchain.d3d11.depth_stencil_view = sapp_d3d11_get_depth_stencil_view();
#endif

        sgl_context_desc_t hud_context_desc;
        EPI_CLEAR_MEMORY(&hud_context_desc, sgl_context_desc_t, 1);
        hud_context_desc.color_format = SG_PIXELFORMAT_RGBA8;
        hud_context_desc.depth_format = SG_PIXELFORMAT_DEPTH;
        hud_context_desc.sample_count = 1;
        hud_context_desc.max_commands = 16384;
        hud_context_desc.max_vertices = 64 * 1024;
        hud_context_                  = sgl_make_context(&hud_context_desc);

        // 2D
        sgl_context_desc_t world_context_desc;
        EPI_CLEAR_MEMORY(&world_context_desc, sgl_context_desc_t, 1);
        world_context_desc.color_format = SG_PIXELFORMAT_RGBA8;
        world_context_desc.depth_format = SG_PIXELFORMAT_DEPTH;
        world_context_desc.sample_count = 1;
        world_context_desc.max_commands = kContextMaxCommand;
        world_context_desc.max_vertices = kContextMaxVertex;

        for (int32_t i = 0; i < kRenderWorldMax; i++)
        {
            for (int32_t j = 0; j < kContextPoolSize; j++)
            {
                world_render_[i].context_pool_[j] = sgl_make_context(&world_context_desc);
            }
        }

        InitPipelines();
        InitImages();

        EPI_CLEAR_MEMORY(world_state_, WorldState, kRenderWorldMax);

        EPI_CLEAR_MEMORY(&render_state_, RenderState, 1);
        render_state_.world_state_ = kWorldStateInvalid;

        RenderBackend::Init();

        
        // clang-format off

        // Quad (Inverted for GL)
        float quad_vertices_uvs[] = {-1.0f, -1.0f, 0.0f, 0, 0, 1.0f, -1.0f, 0.0f, 1, 0, 1.0f,  1.0f, 0.0f, 1, 1,
                                     -1.0f, -1.0f, 0.0f, 0, 0, 1.0f, 1.0f,  0.0f, 1, 1, -1.0f, 1.0f, 0.0f, 0, 1};

        // clang-format on

        sg_buffer_desc quad_desc = {0};
        quad_desc.data           = SG_RANGE(quad_vertices_uvs);
        quad_desc.usage          = SG_USAGE_IMMUTABLE;
        quad_buffer_             = sg_make_buffer(&quad_desc);

        // Screen
        sg_pipeline_desc pip_desc = {0};
        pip_desc.shader           = sg_make_shader(screen_shader_desc(sg_query_backend()));

        pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
        pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;

        screen_pipeline_ = sg_make_pipeline(&pip_desc);

        BSPStartThread();
    }

    // FIXME: go away!
    void GetPassInfo(PassInfo &info)
    {
        RenderLayer layer = GetRenderLayer();

        if (layer == kRenderLayerHUD)
        {
            info.width_  = hud_pass_.swapchain.width;
            info.height_ = hud_pass_.swapchain.height;
        }
        else
        {
            // TODO
            info.width_  = view_window_width;
            info.height_ = view_window_width;
        }
    }

    void SetClearColor(RGBAColor color)
    {
        clear_color_ = color;
    }

    int32_t GetHUDLayer()
    {
        return kRenderLayerHUD;
    }

    void SetupMatrices(RenderLayer layer, bool context_change = false)
    {
        if (layer == kRenderLayerHUD)
        {
            SetupMatrices2D();
        }
        else if (layer == kRenderLayerSky && context_change)
        {
            SetupSkyMatrices();
        }
        else
        {
            SetupMatrices3D();
        }
    }

    void FlushContext()
    {
        if (GetRenderLayer() == kRenderLayerHUD || !render_state_.world_render_)
        {
            return;
        }

        if (sgl_num_vertices())
        {
            sgl_context_draw(world_render_->context_pool_[world_render_->current_context_]);
        }

        world_render_->current_context_++;
        EPI_ASSERT(world_render_->current_context_ < kContextPoolSize);

        sgl_set_context(world_render_->context_pool_[world_render_->current_context_]);
        render_state->OnContextSwitch();

        SetupMatrices(render_state_.layer_, true);
    }

    virtual void SetRenderLayer(RenderLayer layer, bool clear_depth = false)
    {
        render_state_.layer_ = layer;

        if (layer == kRenderLayerHUD)
        {
            sgl_set_context(hud_context_);
        }
        else
        {
            EPI_ASSERT(render_state_.world_render_);
        }

        SetupMatrices(layer);

        if (clear_depth)
        {
            sgl_clear_depth(1.0f);
        }
    }

    RenderLayer GetRenderLayer()
    {
        return render_state_.layer_;
    }

    void InitWorldRender(WorldRender *render)
    {
        if (render->color_target_.id != SG_INVALID_ID)
        {
            return;
        }

        sg_sampler_desc smp_desc = {0};
        smp_desc.min_filter      = SG_FILTER_LINEAR;
        smp_desc.mag_filter      = SG_FILTER_LINEAR;

        sg_image_desc img_color_desc = {0};
        img_color_desc.render_target = true;
        img_color_desc.width         = current_screen_width;
        img_color_desc.height        = current_screen_height;
        img_color_desc.pixel_format  = SG_PIXELFORMAT_RGBA8;
        img_color_desc.sample_count  = 1;
        img_color_desc.label         = "Color Target";

        render->color_target_  = sg_make_image(&img_color_desc);
        render->color_sampler_ = sg_make_sampler(&smp_desc);

        sg_image_desc img_depth_desc = {0};
        img_depth_desc.render_target = true;
        img_depth_desc.width         = current_screen_width;
        img_depth_desc.height        = current_screen_height;
        img_depth_desc.pixel_format  = SG_PIXELFORMAT_DEPTH;
        img_depth_desc.sample_count  = 1;
        img_depth_desc.label         = "Depth Target";

        render->depth_target_  = sg_make_image(&img_depth_desc);
        render->depth_sampler_ = sg_make_sampler(&smp_desc);

        sg_attachments_desc attachments = {0};
        attachments.colors[0].image     = render->color_target_;
        attachments.depth_stencil.image = render->depth_target_;
        attachments.label               = "World Attachments";

        render->world_pass_ = {0};

        sg_pass_action pass_action;
        pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass_action.colors[0].clear_value = {epi::GetRGBARed(clear_color_) / 255.0f, 0.0f,
                                             epi::GetRGBABlue(clear_color_) / 255.0f, 1.0f};

        pass_action.depth.load_action = SG_LOADACTION_CLEAR;
        pass_action.depth.clear_value = 1.0f;
        pass_action.stencil           = {SG_LOADACTION_CLEAR, SG_STOREACTION_DONTCARE, 0};

        render->world_pass_.action      = pass_action;
        render->world_pass_.attachments = sg_make_attachments(&attachments);
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

        WorldRender *render = &world_render_[i];

        render_state_.world_render_ = render;

        InitWorldRender(render);

        render->current_context_ = 0;
        sg_begin_pass(render->world_pass_);
        sgl_set_context(render->context_pool_[0]);
    }

    void FinishWorldRender()
    {
        WorldRender *render         = render_state_.world_render_;
        render_state_.world_state_  = kWorldStateInvalid;
        render_state_.world_render_ = nullptr;

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

        if (sgl_num_vertices())
        {
            sgl_context_draw(render->context_pool_[world_render_->current_context_]);
        }

        sg_end_pass();

        SetRenderLayer(kRenderLayerHUD);
    }

    void GetFrameStats(FrameStats &stats)
    {
        sg_frame_stats sg_stats = sg_query_frame_stats();

        stats.num_apply_pipeline_ = sg_stats.num_apply_pipeline;
        stats.num_apply_bindings_ = sg_stats.num_apply_bindings;
        stats.num_apply_uniforms_ = sg_stats.num_apply_uniforms;
        stats.num_draw_           = sg_stats.num_draw;
        stats.num_update_buffer_  = sg_stats.num_update_buffer;
        stats.num_update_image_   = sg_stats.num_update_image;

        stats.size_apply_uniforms_ = sg_stats.size_apply_uniforms;
        stats.size_update_buffer_  = sg_stats.size_update_buffer;
        stats.size_append_buffer_  = sg_stats.size_append_buffer;
    }

  private:
    struct RenderState
    {
        RenderLayer  layer_;
        int32_t      world_state_;
        WorldRender *world_render_;
    };

    struct WorldRender
    {
        sg_pass world_pass_;

        sg_image   color_target_;
        sg_sampler color_sampler_;
        sg_image   depth_target_;
        sg_sampler depth_sampler_;

        sgl_context context_pool_[kContextPoolSize];
        int32_t     current_context_;

        sg_pass     gbuffer_pass_;
        sg_image    gbuffer_target_;
        sg_pipeline gbuffer_pipeline_;

        sgl_context gbuffer_context_;
    };

    struct WorldState
    {
        bool active_;
        bool used_;
    };

    RGBAColor clear_color_ = kRGBABlack;

    RenderState render_state_;

    sg_pass     hud_pass_;
    sgl_context hud_context_;

    // Screen

    sg_pipeline screen_pipeline_;
    sg_buffer   quad_buffer_;

    WorldState  world_state_[kRenderWorldMax];
    WorldRender world_render_[kRenderWorldMax];

#ifdef SOKOL_D3D11
    bool    deferred_resize        = false;
    int32_t deferred_resize_width  = 0;
    int32_t deferred_resize_height = 0;
#endif
};

static SokolRenderBackend sokol_render_backend;
RenderBackend            *render_backend = &sokol_render_backend;
