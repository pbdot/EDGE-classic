#include "../../r_backend.h"
// clang-format off
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

#include "thread.h"

// clang-format on

struct RenderData
{
    sg_pass     pass_;
    int64_t     frame_number_;
    sgl_context context_;
    WorldRender world_render_[kRenderWorldMax];
};

struct RenderThread
{
    thread_ptr_t        thread_;
    thread_atomic_int_t exit_flag_;
    thread_atomic_ptr_t render_data_;
    thread_mutex_t      render_mutex_;
};

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

        EPI_CLEAR_MEMORY(render_data_current_->world_render_, WorldRender, kRenderWorldMax);
        sgl_layer(kRenderLayerHUD);
    }

    void SwapBuffers()
    {
#ifdef SOKOL_D3D11
        // sapp_d3d11_present(false);
#endif
    }

    void RenderThreadStart()
    {        
        thread_atomic_int_store(&render_thread_.exit_flag_, 0);
        thread_atomic_ptr_store(&render_thread_.render_data_, nullptr);
        thread_mutex_init(&render_thread_.render_mutex_);

        render_thread_.thread_ = thread_create(RenderThreadProc, &render_thread_, THREAD_STACK_SIZE_DEFAULT);
    }
    void RenderThreadStop()
    {
        thread_mutex_term(&render_thread_.render_mutex_);
        thread_atomic_int_store(&render_thread_.exit_flag_, 1);
        thread_join(render_thread_.thread_);
    }

    static int32_t RenderThreadProc(void *thread_data)
    {
        RenderThread *thread = (RenderThread *)thread_data;

        while (thread_atomic_int_load(&thread->exit_flag_) == 0)
        {
            RenderData *data = (RenderData *)thread_atomic_ptr_load(&thread->render_data_);

            if (data)
            {                
                sg_begin_pass(&data->pass_);

                for (int32_t i = 0; i < kRenderWorldMax; i++)
                {
                    WorldRender *world_render = &data->world_render_[i];
                    if (world_render->active_)
                    {
                        FatalError("SokolRenderBackend: FinishFrame called with active world");
                    }

                    if (!world_render->used_)
                    {
                        break;
                    }

                    for (int32_t j = 0; j < kWorldLayerMax; j++)
                    {
                        sgl_context_draw_layer(data->context_, world_render->layers_[j]);
                    }
                }

                sgl_context_draw_layer(data->context_, kRenderLayerHUD);
                sgl_context_draw_layer(data->context_, 0);

                /*
                sg_imgui_.caps_window.open        = false;
                sg_imgui_.buffer_window.open      = false;
                sg_imgui_.pipeline_window.open    = false;
                sg_imgui_.attachments_window.open = false;
                sg_imgui_.frame_stats_window.open = false;

                simgui_new_frame(&imgui_frame_desc_);
                sgimgui_draw(&sg_imgui_);

                simgui_render();
                */

                sg_end_pass();
                sg_commit();
                sgl_rewind_context(data->context_);                

                sapp_d3d11_present(true);

                thread_atomic_ptr_store(&thread->render_data_, nullptr);
                

                /*
                for (auto itr = on_frame_finished_.begin(); itr != on_frame_finished_.end(); itr++)
                {
                    (*itr)();
                }

                on_frame_finished_.clear();
                */
            }
        }

        return 0;
    }

    void ToggleRenderData()
    {
        if (render_data_current_ == nullptr || render_data_current_ == &render_data_[1])
        {
            render_data_current_ = &render_data_[0];
        }
        else
        {
            render_data_current_ = &render_data_[1];
        }

        sgl_set_context(render_data_current_->context_);
    }

    void FinishFrame()
    {
        sg_pass_action pass_action;
        pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass_action.colors[0].clear_value = {0, 0, 0, 1.0f};

        pass_action.depth.load_action = SG_LOADACTION_CLEAR;
        pass_action.depth.clear_value = 1.0f;

        sg_pass *pass = &render_data_current_->pass_;

        EPI_CLEAR_MEMORY(pass, sg_pass, 1);
        pass->action                   = pass_action;
        pass->swapchain.width          = 1920;
        pass->swapchain.height         = 1080;
        pass->swapchain.color_format   = SG_PIXELFORMAT_RGBA8;
        pass->swapchain.depth_format   = SG_PIXELFORMAT_DEPTH;
        pass->swapchain.gl.framebuffer = 0;

#ifdef SOKOL_D3D11
        pass->swapchain.d3d11.render_view        = sapp_d3d11_get_render_view();
        pass->swapchain.d3d11.resolve_view       = sapp_d3d11_get_resolve_view();
        pass->swapchain.d3d11.depth_stencil_view = sapp_d3d11_get_depth_stencil_view();
#endif

        imgui_frame_desc_            = {0};
        imgui_frame_desc_.width      = 1920;
        imgui_frame_desc_.height     = 1080;
        imgui_frame_desc_.delta_time = 100;
        imgui_frame_desc_.dpi_scale  = 1;

        RenderData* other_data = render_data_current_ == &render_data_[0] ? &render_data_[1] : &render_data_[0]; 
                
        while (thread_atomic_ptr_load(&render_thread_.render_data_) == other_data)
        {            
            //SDL_Delay(1);
        }

        thread_atomic_ptr_store(&render_thread_.render_data_, render_data_current_);
        
        ToggleRenderData();

        /*
        int produced = thread_queue_produce(&render_thread_.render_queue_, render_data_current_, 0);
        if (!produced)
        {
            sgl_rewind_context(render_data_current_->context_);
        }
        // if (!render_data_current_->world_render_[0].used_)
        // SDL_Delay(5);
        */
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

        RenderThreadStop();

#ifdef SOKOL_D3D11
        sapp_d3d11_destroy_device_and_swapchain();
#endif
    }

#ifdef SOKOL_GLCORE
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
#ifdef SOKOL_GLCORE
        CaptureScreenGL(width, height, stride, dest);
#endif

#ifdef SOKOL_D3D11
        sapp_d3d11_capture_screen(width, height, stride, dest);
#endif
    }

    /*
    void BumpContext()
    {
        render_data_current_ = &render_queue_data_[render_data_counter_];
        render_data_counter_++;
        render_data_counter_ %= kMaxRenderQueue;
        render_data_current_->frame_number_ = frame_number_;
        sgl_set_context(render_data_current_->context_);
    }
    */

    void Init()
    {
        LogPrint("Sokol: Initialising...\n");

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
        desc.pipeline_pool_size = 512;
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
        sgl_desc.pipeline_pool_size = 512;
        sgl_desc.context_pool_size  = 3;
        sgl_desc.logger.func        = slog_func;
        sgl_setup(&sgl_desc);

        // 2D
        sgl_context_desc_t context_desc;
        EPI_CLEAR_MEMORY(&context_desc, sgl_context_desc_t, 1);
        context_desc.color_format = SG_PIXELFORMAT_RGBA8;
        context_desc.depth_format = SG_PIXELFORMAT_DEPTH;
        context_desc.sample_count = 1;
        context_desc.max_commands = 256 * 1024;
        context_desc.max_vertices = 1024 * 1024;

        EPI_CLEAR_MEMORY(render_data_, RenderData, 2 );

        render_data_[0].context_   = sgl_make_context(&context_desc);
        render_data_[1].context_   = sgl_make_context(&context_desc);

        // IMGUI
        simgui_desc_t imgui_desc = {0};
        imgui_desc.logger.func   = slog_func;
        simgui_setup(&imgui_desc);

        const sgimgui_desc_t sg_imgui_desc = {0};
        sgimgui_init(&sg_imgui_, &sg_imgui_desc);

        ToggleRenderData();

        InitPipelines();
        InitImages();

        RenderBackend::Init();

        RenderThreadStart();
    }

    void GetPassInfo(PassInfo &info)
    {
        info.width_  = 1920;
        info.height_ = 1080;
    }

    void SetClearColor(RGBAColor color)
    {
        clear_color_ = color;
    }

    int32_t GetHUDLayer()
    {
        return kRenderLayerHUD;
    }

    virtual void SetWorldLayer(WorldLayer layer, bool clear_depth = false)
    {
        WorldRender *world_render    = CurrentWorldRender();
        world_render->current_layer_ = layer;
        sgl_layer(GetCurrentSokolLayer());
        if (clear_depth)
        {
            sgl_clear_depth(1.0f);
        }
    }

    virtual int32_t GetCurrentSokolLayer()
    {
        const WorldRender *world_render = CurrentWorldRender();
        if (!world_render)
        {
            return kRenderLayerHUD;
        }
        return world_render->layers_[world_render->current_layer_];
    }

    WorldRender *BeginWorldRender()
    {
        for (int32_t i = 0; i < kRenderWorldMax; i++)
        {
            WorldRender *world_render = &render_data_current_->world_render_[i];
            if (world_render->active_)
            {
                FatalError("SokolRenderBackend: BeginWorldRender called with active world");
            }

            if (!world_render->used_)
            {
                world_render->active_ = world_render->used_ = true;
                int32_t current_layer                       = kRenderLayerHUD + i * kWorldLayerMax + 1;
                for (int32_t layer = 0; layer < (int32_t)kWorldLayerMax; layer++)
                {
                    world_render->layers_[(WorldLayer)layer] = current_layer++;
                }
                return world_render;
            }
        }

        FatalError("SokolRenderBackend: Max render worlds exceeded");

        return nullptr;
    }

    WorldRender *CurrentWorldRender()
    {
        if (!render_data_current_)
        {
            return nullptr;
        }

        for (int32_t i = 0; i < kRenderWorldMax; i++)
        {
            if (render_data_current_->world_render_[i].active_)
            {
                return &render_data_current_->world_render_[i];
            }
        }

        return nullptr;
    }

    void FinishWorldRender()
    {
        sgl_layer(kRenderLayerHUD);
        SetupWorldMatrices2D();

        for (int32_t i = 0; i < kRenderWorldMax; i++)
        {
            if (render_data_current_->world_render_[i].active_)
            {
                render_data_current_->world_render_[i].active_ = false;
                return;
            }
        }

        FatalError("SokolRenderBackend: FinishWorldRender called with no active world render");
    }

  private:
#ifdef SOKOL_D3D11
    bool    deferred_resize        = false;
    int32_t deferred_resize_width  = 0;
    int32_t deferred_resize_height = 0;
#endif

    simgui_frame_desc_t imgui_frame_desc_;
    sgimgui_t           sg_imgui_;

    RGBAColor clear_color_ = kRGBABlack;

    RenderThread render_thread_;    

    RenderData render_data_[2];
    RenderData *render_data_current_ = nullptr;
};

static SokolRenderBackend sokol_render_backend;
RenderBackend            *render_backend = &sokol_render_backend;
