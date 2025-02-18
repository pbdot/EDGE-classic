// clang-format off

#include <random>

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
#include "shaders/shader_linear_depth.h"
#include "shaders/shader_ssao.h"
#include "shaders/shader_depthblur.h"
#include "shaders/shader_ssao_combine.h"

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

// computes the derived normal matrix for the view matrix
static void ComputeNormalMatrix(float *matrix_out, const float *view_matrix)
{

    double matrix_3x3[9];

    matrix_3x3[0] = view_matrix[0];
    matrix_3x3[1] = view_matrix[1];
    matrix_3x3[2] = view_matrix[2];

    matrix_3x3[3] = view_matrix[4];
    matrix_3x3[4] = view_matrix[5];
    matrix_3x3[5] = view_matrix[6];

    matrix_3x3[6] = view_matrix[8];
    matrix_3x3[7] = view_matrix[9];
    matrix_3x3[8] = view_matrix[10];

    double det, invDet;

    det = matrix_3x3[0] * (matrix_3x3[4] * matrix_3x3[8] - matrix_3x3[5] * matrix_3x3[7]) +
          matrix_3x3[1] * (matrix_3x3[5] * matrix_3x3[6] - matrix_3x3[8] * matrix_3x3[3]) +
          matrix_3x3[2] * (matrix_3x3[3] * matrix_3x3[7] - matrix_3x3[4] * matrix_3x3[6]);

    invDet = 1.0 / det;

    matrix_out[0]  = (matrix_3x3[4] * matrix_3x3[8] - matrix_3x3[5] * matrix_3x3[7]) * invDet;
    matrix_out[1]  = (matrix_3x3[5] * matrix_3x3[6] - matrix_3x3[8] * matrix_3x3[3]) * invDet;
    matrix_out[2]  = (matrix_3x3[3] * matrix_3x3[7] - matrix_3x3[4] * matrix_3x3[6]) * invDet;
    matrix_out[3]  = 0.0f;
    matrix_out[4]  = (matrix_3x3[2] * matrix_3x3[7] - matrix_3x3[1] * matrix_3x3[8]) * invDet;
    matrix_out[5]  = (matrix_3x3[0] * matrix_3x3[8] - matrix_3x3[2] * matrix_3x3[6]) * invDet;
    matrix_out[6]  = (matrix_3x3[1] * matrix_3x3[6] - matrix_3x3[7] * matrix_3x3[0]) * invDet;
    matrix_out[7]  = 0.0f;
    matrix_out[8]  = (matrix_3x3[1] * matrix_3x3[5] - matrix_3x3[4] * matrix_3x3[2]) * invDet;
    matrix_out[9]  = (matrix_3x3[2] * matrix_3x3[3] - matrix_3x3[0] * matrix_3x3[5]) * invDet;
    matrix_out[10] = (matrix_3x3[0] * matrix_3x3[4] - matrix_3x3[3] * matrix_3x3[1]) * invDet;
    matrix_out[11] = 0.0;
    matrix_out[12] = 0.0;
    matrix_out[13] = 0.0;
    matrix_out[14] = 0.0;
    matrix_out[15] = 1.0;
}

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

        HMM_Mat4 proj;

        float fnear = 5.0f;     // renderer_near_clip.f_;
        float ffar  = 65536.0f; // renderer_far_clip.f_;

        float left   = -view_x_slope * fnear;
        float right  = view_x_slope * fnear;
        float bottom = -view_y_slope * fnear;
        float top    = view_y_slope * fnear;

        float A = (right + left) / (right - left);
        float B = (top + bottom) / (top - bottom);
        float C = (-(ffar + fnear)) / (ffar - fnear);
        float D = (-(2 * ffar * fnear)) / (ffar - fnear);

        proj.Columns[0][0] = (2 * fnear) / (right - left);
        proj.Columns[0][1] = 0.0f;
        proj.Columns[0][2] = 0.0f;
        proj.Columns[0][3] = 0.0f;

        proj.Columns[1][0] = 0.0f;
        proj.Columns[1][1] = (2 * fnear) / (top - bottom);
        proj.Columns[1][2] = 0.0f;
        proj.Columns[1][3] = 0.0f;

        proj.Columns[2][0] = A;
        proj.Columns[2][1] = B;
        proj.Columns[2][2] = C;
        proj.Columns[2][3] = -1.0f;

        proj.Columns[3][0] = 0.0f;
        proj.Columns[3][1] = 0.0f;
        proj.Columns[3][2] = D;
        proj.Columns[3][3] = 0.0f;

        // sgl_load_matrix((float *)&proj.Elements[0][0]);

        sgl_frustum(-view_x_slope * 5.0f /*renderer_near_clip.f_*/, view_x_slope * 5.0f /*renderer_near_clip.f_*/,
                    -view_y_slope * 5.0f /*renderer_near_clip.f_*/, view_y_slope * 5.0f /*renderer_near_clip.f_*/,
                    5.0f /*renderer_near_clip.f_*/, 65536.0f /*renderer_far_clip.f_*/);

        sgl_load_matrix(proj.Elements[0]);

        // calculate look-at matrix
        sgl_matrix_mode_modelview();
        /*
        sgl_load_identity();
        sgl_rotate(sgl_rad(270.0f) - epi::RadiansFromBAM(view_vertical_angle), 1.0f, 0.0f, 0.0f);
        sgl_rotate(sgl_rad(90.0f) - epi::RadiansFromBAM(view_angle), 0.0f, 0.0f, 1.0f);
        sgl_translate(-view_x, -view_y, -view_z);
        */

        HMM_Mat4 view = HMM_M4D(1.0f);
        view          = HMM_Mul(view, HMM_Rotate_RH(HMM_AngleDeg(270.0f) - epi::RadiansFromBAM(view_vertical_angle),
                                                    HMM_V3(1.0f, 0.0f, 0.0f)));
        view          = HMM_Mul(view,
                                HMM_Rotate_RH(HMM_AngleDeg(90.0f) - epi::RadiansFromBAM(view_angle), HMM_V3(0.0f, 0.0f, 1.0f)));
        view          = HMM_Mul(view, HMM_Translate(HMM_V3(-view_x, -view_y, -view_z)));

        sgl_load_matrix(view.Elements[0]);

        HMM_Mat4 normal_matrix;
        ComputeNormalMatrix((float *)&normal_matrix, (float *)&view);

        sgl_matrix_mode_texture();
        sgl_load_matrix(normal_matrix.Elements[0]);
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
        sg_begin_pass(&render->linear_depth_pass_);

        // Linear Depth
        sg_apply_pipeline(linear_depth_pipeline_);

        sg_bindings bind = {0};

        bind.vertex_buffers[0] = quad_buffer_;
        bind.images[0]         = render->depth_target_;
        bind.samplers[0]       = render->depth_sampler_;
        bind.images[1]         = render->color_target_;
        bind.samplers[1]       = render->color_sampler_;

        sg_apply_bindings(&bind);

        linear_depth_params_t fs_params;
        fs_params.linearize_depth_a = 1.0f / /*renderer_far_clip.f_*/ 65536.0f - 1.0f / 5.0f /*renderer_near_clip.f_*/;
        fs_params.linearize_depth_b = HMM_MAX(1.0f / 5.0f /*renderer_near_clip.f_*/, 1.e-8f);
        fs_params.inverse_depth_range_a = 1.0f;
        fs_params.inverse_depth_range_b = 0.0f;

        sg_range range = SG_RANGE(fs_params);
        sg_apply_uniforms(UB_linear_depth_params, &range);

        sg_draw(0, 6, 1);

        sg_end_pass();

        // SSAO

        sg_begin_pass(&render->ssao_pass_);

        sg_apply_pipeline(ssao_pipeline_);

        bind = {0};

        bind.vertex_buffers[0] = quad_buffer_;
        bind.images[0]         = render->linear_depth_target_;
        bind.samplers[0]       = render->linear_depth_sampler_;
        bind.images[1]         = render->normal_target_;
        bind.samplers[1]       = render->normal_sampler_;
        bind.images[2]         = random_texture_;
        bind.samplers[2]       = random_sampler_;

        sg_apply_bindings(&bind);

        ssao_params_t ssao_params;

        // cvars
        float bias       = 0.2;
        float aoRadius   = 80.0f;
        float aoStrength = 0.85f;

        HMM_Mat4 proj;

        float fnear = 5.0f;     // renderer_near_clip.f_;
        float ffar  = 65536.0f; // renderer_far_clip.f_;

        float left   = -view_x_slope * fnear;
        float right  = view_x_slope * fnear;
        float bottom = -view_y_slope * fnear;
        float top    = view_y_slope * fnear;

        float A = (right + left) / (right - left);
        float B = (top + bottom) / (top - bottom);
        float C = (-(ffar + fnear)) / (ffar - fnear);
        float D = (-(2 * ffar * fnear)) / (ffar - fnear);

        proj.Columns[0][0] = (2 * fnear) / (right - left);
        proj.Columns[0][1] = 0.0f;
        proj.Columns[0][2] = 0.0f;
        proj.Columns[0][3] = 0.0f;

        proj.Columns[1][0] = 0.0f;
        proj.Columns[1][1] = (2 * fnear) / (top - bottom);
        proj.Columns[1][2] = 0.0f;
        proj.Columns[1][3] = 0.0f;

        proj.Columns[2][0] = A;
        proj.Columns[2][1] = B;
        proj.Columns[2][2] = C;
        proj.Columns[2][3] = -1.0f;

        proj.Columns[3][0] = 0.0f;
        proj.Columns[3][1] = 0.0f;
        proj.Columns[3][2] = D;
        proj.Columns[3][3] = 0.0f;

        int32_t AmbientWidth  = (current_screen_width + 1) / 2;
        int32_t AmbientHeight = (current_screen_height + 1) / 2;

        // TODO!: m5 = VPUniforms.mProjectionMatrix.get()[5]
        // float m5           = ((float *)&proj)[5];

        float m5 = proj.Columns[1][1];

        float tanHalfFovy  = 1.0f / m5;
        float invFocalLenX = tanHalfFovy * ((float)AmbientWidth / (float)AmbientHeight);
        float invFocalLenY = tanHalfFovy;
        float nDotVBias    = HMM_Clamp(bias, 0.0f, 1.0f);
        float r2           = aoRadius * aoRadius;

        ssao_params.SampleIndex          = 0;
        ssao_params.UVToViewA[0]         = 2.0f * invFocalLenX;
        ssao_params.UVToViewA[1]         = 2.0f * invFocalLenY;
        ssao_params.UVToViewB[0]         = -invFocalLenX;
        ssao_params.UVToViewB[1]         = -invFocalLenY;
        ssao_params.InvFullResolution[0] = 1.0f / AmbientWidth;
        ssao_params.InvFullResolution[1] = 1.0f / AmbientHeight;
        ssao_params.NDotVBias            = nDotVBias;
        ssao_params.NegInvR2             = -1.0f / r2;
        ssao_params.RadiusToScreen       = aoRadius * 0.5f / tanHalfFovy * AmbientHeight;
        ssao_params.AOMultiplier         = 1.0f / (1.0f - nDotVBias);
        ssao_params.AOStrength           = aoStrength;

        /*
            FVector2 SceneScale() const
            {
                return { mSceneViewport.width / (float)mScreenViewport.width, mSceneViewport.height /
           (float)mScreenViewport.height };
            }

            FVector2 SceneOffset() const
            {
                return { mSceneViewport.left / (float)mScreenViewport.width, mSceneViewport.top /
           (float)mScreenViewport.height };
            }

        */
        ssao_params.Scale[0]  = 1.0f;
        ssao_params.Scale[1]  = 1.0f;
        ssao_params.Offset[0] = 0.0f;
        ssao_params.Offset[1] = 0.0f;

        range = SG_RANGE(ssao_params);
        sg_apply_uniforms(UB_ssao_params, &range);

        sg_draw(0, 6, 1);

        sg_end_pass();

        // Blur
        depthblur_params_t depthblur_params;
        const float        blurAmount       = 15.0f;
        const float        gl_ssao_exponent = 1.8f; // can be set in console, mark the other that can be too

        // Horizontal
        sg_begin_pass(&render->depthblur_horizontal_pass_);
        sg_apply_pipeline(depthblur_pipeline_);

        bind = {0};

        bind.vertex_buffers[0] = quad_buffer_;
        bind.images[0]         = render->ambient0_target_;
        bind.samplers[0]       = render->ambient0_sampler_;

        sg_apply_bindings(&bind);

        depthblur_params.direction     = 0;
        depthblur_params.BlurSharpness = 1.0f / blurAmount;
        depthblur_params.PowExponent   = gl_ssao_exponent;

        range = SG_RANGE(depthblur_params);
        sg_apply_uniforms(UB_depthblur_params, &range);

        sg_draw(0, 6, 1);

        sg_end_pass();

        // Vertical
        sg_begin_pass(&render->depthblur_vertical_pass_);
        sg_apply_pipeline(depthblur_pipeline_);

        bind = {0};

        bind.vertex_buffers[0] = quad_buffer_;
        bind.images[0]         = render->ambient1_target_;
        bind.samplers[0]       = render->ambient1_sampler_;

        sg_apply_bindings(&bind);

        depthblur_params.direction     = 1;
        depthblur_params.BlurSharpness = 1.0f / blurAmount;
        depthblur_params.PowExponent   = gl_ssao_exponent;

        range = SG_RANGE(depthblur_params);
        sg_apply_uniforms(UB_depthblur_params, &range);

        sg_draw(0, 6, 1);

        sg_end_pass();

        // SSAO Combine

        sg_begin_pass(&render->ssao_combine_pass_);
        sg_apply_pipeline(ssao_combine_pipeline_);

        bind = {0};

        bind.vertex_buffers[0] = quad_buffer_;
        bind.images[0]         = render->ambient0_target_;
        bind.samplers[0]       = render->ambient0_sampler_;

        // this is supposed to be fog
        bind.images[1]   = render->color_target_;
        bind.samplers[1] = render->color_sampler_;

        sg_apply_bindings(&bind);

        sg_draw(0, 6, 1);

        sg_end_pass();

        // Screen

        sg_begin_pass(&screen_pass_);

        sg_apply_pipeline(screen_pipeline_);

        bind = {0};

        bind.vertex_buffers[0] = quad_buffer_;
        bind.images[0]         = render->final_target_;
        bind.samplers[0]       = render->final_sampler_;

        sg_apply_bindings(&bind);

        sg_draw(0, 6, 1);

        sg_end_pass();
    }

    void RenderHud()
    {
        // hack for hud flip
        sgl_point_size(1);
        for (int32_t i = 0; i < kRenderWorldMax; i++)
        {
            if (world_state_[i].used_)
            {
                RenderWorldToScreen(&world_render_[i]);
            }
        }

        // hack for hud flip
        sgl_point_size(0);
        sg_begin_pass(&hud_pass_);

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
        pass_action.colors[0].load_action = SG_LOADACTION_DONTCARE;
        // pass_action.colors[0].clear_value = {epi::GetRGBARed(clear_color_) / 255.0f,
        //                                      epi::GetRGBAGreen(clear_color_) / 255.0f,
        //                                      epi::GetRGBABlue(clear_color_) / 255.0f, 1.0f};

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

        float quad_vertices_uvs[] = {-1.0f, -1.0f, 0.0f, 0, 0, 1.0f, -1.0f, 0.0f, 1, 0, 1.0f,  1.0f, 0.0f, 1, 1,
                                     -1.0f, -1.0f, 0.0f, 0, 0, 1.0f, 1.0f,  0.0f, 1, 1, -1.0f, 1.0f, 0.0f, 0, 1};

        sg_buffer_desc quad_desc = {0};
        quad_desc.data           = SG_RANGE(quad_vertices_uvs);
        quad_desc.usage          = SG_USAGE_IMMUTABLE;
        quad_buffer_             = sg_make_buffer(&quad_desc);

        // Screen
        sg_pipeline_desc screen_pip_desc = {0};
        screen_pip_desc.shader           = sg_make_shader(screen_shader_desc(sg_query_backend()));

        screen_pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
        screen_pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;

        screen_pipeline_ = sg_make_pipeline(&screen_pip_desc);

        EPI_CLEAR_MEMORY(&pass_action, sg_pass_action, 1);
        pass_action.colors[0].load_action = SG_LOADACTION_DONTCARE;
        pass_action.depth.load_action     = SG_LOADACTION_DONTCARE;

        EPI_CLEAR_MEMORY(&screen_pass_, sg_pass, 1);
        screen_pass_.action = pass_action;

        screen_pass_.swapchain.width          = current_screen_width;
        screen_pass_.swapchain.height         = current_screen_height;
        screen_pass_.swapchain.color_format   = SG_PIXELFORMAT_RGBA8;
        screen_pass_.swapchain.depth_format   = SG_PIXELFORMAT_DEPTH;
        screen_pass_.swapchain.gl.framebuffer = 0;

#ifdef SOKOL_D3D11
        screen_pass_.swapchain.d3d11.render_view        = sapp_d3d11_get_render_view();
        screen_pass_.swapchain.d3d11.resolve_view       = sapp_d3d11_get_resolve_view();
        screen_pass_.swapchain.d3d11.depth_stencil_view = sapp_d3d11_get_depth_stencil_view();
#endif

        // Linear Depth
        sg_pipeline_desc linear_depth_pip_desc       = {0};
        linear_depth_pip_desc.colors[0].pixel_format = SG_PIXELFORMAT_R32F;
        linear_depth_pip_desc.color_count            = 1;
        linear_depth_pip_desc.depth.pixel_format     = SG_PIXELFORMAT_NONE;

        linear_depth_pip_desc.shader = sg_make_shader(linear_depth_shader_desc(sg_query_backend()));

        linear_depth_pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
        linear_depth_pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;

        linear_depth_pipeline_ = sg_make_pipeline(&linear_depth_pip_desc);

        // SSAO

        sg_pipeline_desc ssao_pip_desc       = {0};
        ssao_pip_desc.colors[0].pixel_format = SG_PIXELFORMAT_RG16F;
        ssao_pip_desc.color_count            = 1;
        ssao_pip_desc.depth.pixel_format     = SG_PIXELFORMAT_NONE;

        ssao_pip_desc.shader = sg_make_shader(ssao_shader_desc(sg_query_backend()));

        ssao_pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
        ssao_pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;

        ssao_pipeline_ = sg_make_pipeline(&ssao_pip_desc);

        // Blur

        sg_pipeline_desc depthblur_pip_desc       = {0};
        depthblur_pip_desc.colors[0].pixel_format = SG_PIXELFORMAT_RG16F;
        depthblur_pip_desc.color_count            = 1;
        depthblur_pip_desc.depth.pixel_format     = SG_PIXELFORMAT_NONE;

        depthblur_pip_desc.shader = sg_make_shader(depthblur_shader_desc(sg_query_backend()));

        depthblur_pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
        depthblur_pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;

        depthblur_pipeline_ = sg_make_pipeline(&depthblur_pip_desc);

        // SSAO Combine

        sg_pipeline_desc ssao_combine_pip_desc       = {0};
        ssao_combine_pip_desc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
        ssao_combine_pip_desc.color_count            = 1;
        ssao_combine_pip_desc.depth.pixel_format     = SG_PIXELFORMAT_NONE;

        ssao_combine_pip_desc.shader = sg_make_shader(ssao_combine_shader_desc(sg_query_backend()));

        ssao_combine_pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
        ssao_combine_pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;

        ssao_combine_pipeline_ = sg_make_pipeline(&ssao_combine_pip_desc);

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
        smp_desc.wrap_u          = SG_WRAP_CLAMP_TO_EDGE;
        smp_desc.wrap_v          = SG_WRAP_CLAMP_TO_EDGE;

        // World Pass

        sg_image_desc img_color_desc = {0};
        img_color_desc.render_target = true;
        img_color_desc.width         = current_screen_width;
        img_color_desc.height        = current_screen_height;
        img_color_desc.pixel_format  = SG_PIXELFORMAT_RGBA8;
        img_color_desc.sample_count  = 1;
        img_color_desc.label         = "Color Target";

        render->color_target_  = sg_make_image(&img_color_desc);
        render->color_sampler_ = sg_make_sampler(&smp_desc);

        render->final_target_  = sg_make_image(&img_color_desc);
        render->final_sampler_ = sg_make_sampler(&smp_desc);

        sg_image_desc img_normal_desc = {0};
        img_normal_desc.render_target = true;
        img_normal_desc.width         = current_screen_width;
        img_normal_desc.height        = current_screen_height;
        img_normal_desc.pixel_format  = SG_PIXELFORMAT_RGB10A2; // GL_RGB10_A2
        img_normal_desc.sample_count  = 1;
        img_normal_desc.label         = "Normal Target";

        render->normal_target_  = sg_make_image(&img_normal_desc);
        render->normal_sampler_ = sg_make_sampler(&smp_desc);

        sg_image_desc img_depth_desc = {0};
        img_depth_desc.render_target = true;
        img_depth_desc.width         = current_screen_width;
        img_depth_desc.height        = current_screen_height;
        img_depth_desc.pixel_format  = SG_PIXELFORMAT_DEPTH;
        img_depth_desc.sample_count  = 1;
        img_depth_desc.label         = "Depth Target";

        sg_sampler_desc depth_smp_desc = {0};
        depth_smp_desc.wrap_u          = SG_WRAP_CLAMP_TO_EDGE;
        depth_smp_desc.wrap_v          = SG_WRAP_CLAMP_TO_EDGE;

        render->depth_target_  = sg_make_image(&img_depth_desc);
        render->depth_sampler_ = sg_make_sampler(&depth_smp_desc);

        sg_attachments_desc world_attachments = {0};
        world_attachments.colors[0].image     = render->color_target_;
        world_attachments.colors[1].image     = render->normal_target_;
        world_attachments.depth_stencil.image = render->depth_target_;
        world_attachments.label               = "World Attachments";

        render->world_pass_ = {0};

        sg_pass_action pass_action;
        EPI_CLEAR_MEMORY(&pass_action, sg_pass_action, 1);

        pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass_action.colors[0].clear_value = {epi::GetRGBARed(clear_color_) / 255.0f,
                                             epi::GetRGBAGreen(clear_color_) / 255.0f,
                                             epi::GetRGBABlue(clear_color_) / 255.0f, 1.0f};

        pass_action.colors[1].load_action = SG_LOADACTION_CLEAR;
        pass_action.colors[1].clear_value = {0.0f, 0.0f, 0.0f, 1.0f};

        pass_action.depth.load_action = SG_LOADACTION_CLEAR;
        pass_action.depth.clear_value = 1.0f;
        pass_action.stencil           = {SG_LOADACTION_CLEAR, SG_STOREACTION_DONTCARE, 0};

        render->world_pass_.action      = pass_action;
        render->world_pass_.attachments = sg_make_attachments(&world_attachments);

        int32_t AmbientWidth  = (current_screen_width + 1) / 2;
        int32_t AmbientHeight = (current_screen_height + 1) / 2;

        // Linear Depth Pass
        sg_image_desc linear_depth_desc = {0};
        linear_depth_desc;
        linear_depth_desc.render_target = true;
        linear_depth_desc.width         = AmbientWidth;
        linear_depth_desc.height        = AmbientHeight;
        linear_depth_desc.pixel_format  = SG_PIXELFORMAT_R32F;
        linear_depth_desc.sample_count  = 1;
        linear_depth_desc.label         = "Linear Depth Target";

        render->linear_depth_target_          = sg_make_image(&linear_depth_desc);
        sg_sampler_desc linear_depth_smp_desc = {0};
        linear_depth_smp_desc.wrap_u          = SG_WRAP_CLAMP_TO_EDGE;
        linear_depth_smp_desc.wrap_v          = SG_WRAP_CLAMP_TO_EDGE;
        render->linear_depth_sampler_         = sg_make_sampler(&linear_depth_smp_desc);

        sg_attachments_desc linear_depth_attachments = {0};
        linear_depth_attachments.colors[0].image     = render->linear_depth_target_;
        linear_depth_attachments.label               = "Linear Depth Attachments";

        render->linear_depth_pass_ = {0};

        EPI_CLEAR_MEMORY(&pass_action, sg_pass_action, 1);
        pass_action.colors[0].load_action = SG_LOADACTION_DONTCARE;

        render->linear_depth_pass_.action      = pass_action;
        render->linear_depth_pass_.attachments = sg_make_attachments(&linear_depth_attachments);

        // SSAO
        sg_image_desc ssao_desc = {0};
        ssao_desc;
        ssao_desc.render_target = true;
        ssao_desc.width         = AmbientWidth;
        ssao_desc.height        = AmbientHeight;
        ssao_desc.pixel_format  = SG_PIXELFORMAT_RG16F;
        ssao_desc.sample_count  = 1;
        ssao_desc.label         = "SSAO Target";

        sg_sampler_desc ambient0_smp_desc = {0};
        ambient0_smp_desc.wrap_u          = SG_WRAP_CLAMP_TO_EDGE;
        ambient0_smp_desc.wrap_v          = SG_WRAP_CLAMP_TO_EDGE;

        render->ambient0_target_  = sg_make_image(&ssao_desc);
        render->ambient0_sampler_ = sg_make_sampler(&ambient0_smp_desc);
        // for blur
        render->ambient1_target_  = sg_make_image(&ssao_desc);
        render->ambient1_sampler_ = sg_make_sampler(&ambient0_smp_desc);

        sg_attachments_desc ssao_attachments = {0};
        ssao_attachments.colors[0].image     = render->ambient0_target_;
        ssao_attachments.label               = "SSAO Attachments";

        render->ssao_pass_ = {0};

        EPI_CLEAR_MEMORY(&pass_action, sg_pass_action, 1);
        pass_action.colors[0].load_action = SG_LOADACTION_DONTCARE;

        render->ssao_pass_.action      = pass_action;
        render->ssao_pass_.attachments = sg_make_attachments(&ssao_attachments);

        CreateRandomTexture();

        // Blur

        // Horizontal
        sg_attachments_desc depthblur_horizontal_attachments = {0};
        depthblur_horizontal_attachments.colors[0].image     = render->ambient1_target_;
        depthblur_horizontal_attachments.label               = "Blur Horizontal Attachments";

        render->depthblur_horizontal_pass_ = {0};

        EPI_CLEAR_MEMORY(&pass_action, sg_pass_action, 1);
        pass_action.colors[0].load_action = SG_LOADACTION_DONTCARE;

        render->depthblur_horizontal_pass_.action      = pass_action;
        render->depthblur_horizontal_pass_.attachments = sg_make_attachments(&depthblur_horizontal_attachments);

        // Vertical
        render->depthblur_vertical_pass_ = {0};

        sg_attachments_desc depthblur_vertical_attachments = {0};
        depthblur_vertical_attachments.colors[0].image     = render->ambient0_target_;
        depthblur_vertical_attachments.label               = "Blur Horizontal Attachments";

        EPI_CLEAR_MEMORY(&pass_action, sg_pass_action, 1);
        pass_action.colors[0].load_action = SG_LOADACTION_DONTCARE;

        render->depthblur_vertical_pass_.action      = pass_action;
        render->depthblur_vertical_pass_.attachments = sg_make_attachments(&depthblur_vertical_attachments);

        // SSAO Combine
        sg_attachments_desc ssao_combine_attachments = {0};
        ssao_combine_attachments.colors[0].image     = render->final_target_;
        ssao_combine_attachments.label               = "SSAO Combine Attachments";

        render->ssao_combine_pass_ = {0};

        EPI_CLEAR_MEMORY(&pass_action, sg_pass_action, 1);
        pass_action.colors[0].load_action = SG_LOADACTION_DONTCARE;

        render->ssao_combine_pass_.action      = pass_action;
        render->ssao_combine_pass_.attachments = sg_make_attachments(&ssao_combine_attachments);
    }

    void CreateRandomTexture()
    {
        // Must match quality enum in PPAmbientOcclusion::DeclareShaders
        double numDirections = 8;

        std::mt19937                           generator(1337);
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        std::shared_ptr<void>                  data(new int16_t[16 * 4], [](void *p) { delete[] (int16_t *)p; });
        int16_t                               *randomValues = (int16_t *)data.get();

        for (int i = 0; i < 16; i++)
        {
            double angle = 2.0 * M_PI * distribution(generator) / numDirections;
            double x     = cos(angle);
            double y     = sin(angle);
            double z     = distribution(generator);
            double w     = distribution(generator);

            randomValues[i * 4 + 0] = (int16_t)HMM_Clamp(x * 32767.0, -32768.0, 32767.0);
            randomValues[i * 4 + 1] = (int16_t)HMM_Clamp(y * 32767.0, -32768.0, 32767.0);
            randomValues[i * 4 + 2] = (int16_t)HMM_Clamp(z * 32767.0, -32768.0, 32767.0);
            randomValues[i * 4 + 3] = (int16_t)HMM_Clamp(w * 32767.0, -32768.0, 32767.0);
        }

        sg_image_desc img_desc;
        EPI_CLEAR_MEMORY(&img_desc, sg_image_desc, 1);
        img_desc.usage        = SG_USAGE_IMMUTABLE;
        img_desc.width        = 4;
        img_desc.height       = 4;
        img_desc.pixel_format = SG_PIXELFORMAT_RGBA16SN;
        img_desc.num_mipmaps  = 1;
        img_desc.sample_count = 1;

        sg_image_data img_data;
        EPI_CLEAR_MEMORY(&img_data, sg_image_data, 1);

        sg_range range;
        range.ptr               = data.get();
        range.size              = 16 * 4 * 2;
        img_data.subimage[0][0] = range;

        img_desc.data = img_data;

        random_texture_ = sg_make_image(&img_desc);

        sg_sampler_desc random_smp_desc = {0};

        random_smp_desc.wrap_u     = SG_WRAP_REPEAT;
        random_smp_desc.wrap_v     = SG_WRAP_REPEAT;
        random_smp_desc.min_filter = SG_FILTER_NEAREST;
        random_smp_desc.mag_filter = SG_FILTER_NEAREST;

        random_sampler_ = sg_make_sampler(&random_smp_desc);
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
        sg_pass    world_pass_;
        sg_image   color_target_;
        sg_sampler color_sampler_;
        sg_image   depth_target_;
        sg_sampler depth_sampler_;
        sg_image   normal_target_;
        sg_sampler normal_sampler_;

        sg_pass    linear_depth_pass_;
        sg_image   linear_depth_target_;
        sg_sampler linear_depth_sampler_;

        sg_pass    ssao_pass_;
        sg_image   ambient0_target_;
        sg_sampler ambient0_sampler_;
        // for blur
        sg_image   ambient1_target_;
        sg_sampler ambient1_sampler_;

        sg_pass depthblur_horizontal_pass_;
        sg_pass depthblur_vertical_pass_;

        sg_pass ssao_combine_pass_;

        sg_image   final_target_;
        sg_sampler final_sampler_;

        sgl_context context_pool_[kContextPoolSize];
        int32_t     current_context_;
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

    sg_pass     screen_pass_;
    sg_pipeline screen_pipeline_;
    sg_pipeline linear_depth_pipeline_;
    sg_pipeline ssao_pipeline_;
    sg_pipeline depthblur_pipeline_;
    sg_pipeline ssao_combine_pipeline_;
    sg_image    random_texture_;
    sg_sampler  random_sampler_;
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
