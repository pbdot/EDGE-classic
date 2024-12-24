#include "epi_str_compare.h"
#include "g_game.h"
#include "i_defs_gl.h"
#include "i_video.h"
#include "r_colormap.h"
#include "r_draw.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_units.h"
#include "sk_local.h"

#ifdef SOKOL_D3D11
#include "sk_d3d11.h"
#endif

#include "sk_state.h"

extern int maximum_texture_size;

static SokolRenderState state;

RenderState *global_render_state = &state;

void SokolRenderState::Initialize()
{
    sg_environment env;
    memset(&env, 0, sizeof(env));
    env.defaults.color_format = SG_PIXELFORMAT_RGBA8;
    env.defaults.depth_format = SG_PIXELFORMAT_DEPTH;
    env.defaults.sample_count = 1;

#ifdef SOKOL_D3D11
    R_InitD3D11(program_window);
#endif
    sg_desc desc{0};
    desc.environment     = env;
    desc.logger.func     = slog_func;
    desc.image_pool_size = 8192;

#ifdef SOKOL_D3D11
    desc.environment.d3d11.device = sapp_d3d11_get_device();
    desc.environment.d3d11.device_context = sapp_d3d11_get_device_context();
#endif

    sg_setup(&desc);

    if (!sg_isvalid())
    {
        FatalError("Sokol invalid");
    }

    sgl_desc_t sgl_desc;
    memset(&sgl_desc, 0, sizeof(sgl_desc));
    sgl_desc.color_format       = SG_PIXELFORMAT_RGBA8;
    sgl_desc.depth_format       = SG_PIXELFORMAT_DEPTH;
    sgl_desc.sample_count       = 1;
    sgl_desc.pipeline_pool_size = 512;
    sgl_desc.logger.func        = slog_func;
    sgl_setup(&sgl_desc);

    // Default sampler
    sg_sampler_desc sdesc = {0};

    sdesc.wrap_u = SG_WRAP_REPEAT;
    sdesc.wrap_v = SG_WRAP_REPEAT;

    // filtering
    sdesc.mag_filter    = SG_FILTER_NEAREST;
    sdesc.min_filter    = SG_FILTER_NEAREST;
    sdesc.mipmap_filter = SG_FILTER_NEAREST;

    default_sampler = sg_make_sampler(&sdesc);

    // Clamp sampler
    sdesc = {0};

    sdesc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    sdesc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;

    // filtering
    sdesc.mag_filter    = SG_FILTER_NEAREST;
    sdesc.min_filter    = SG_FILTER_NEAREST;
    sdesc.mipmap_filter = SG_FILTER_NEAREST;

    clamp_sampler = sg_make_sampler(&sdesc);

    // 2D
    sgl_context_desc_t context_desc_2d = {0};
    context_desc_2d.color_format       = SG_PIXELFORMAT_RGBA8;
    context_desc_2d.depth_format       = SG_PIXELFORMAT_DEPTH;
    context_desc_2d.sample_count       = 1;
    context_desc_2d.max_commands       = 16 * 1024;
    context_desc_2d.max_vertices       = 128 * 1024;

    context_2d_ = sgl_make_context(&context_desc_2d);

    // 3D
    sgl_context_desc_t context_desc_3d = {0};
    context_desc_3d.color_format       = SG_PIXELFORMAT_RGBA8;
    context_desc_3d.depth_format       = SG_PIXELFORMAT_DEPTH;
    context_desc_3d.sample_count       = 1;
    context_desc_3d.max_commands       = 16 * 1024;
    context_desc_3d.max_vertices       = 128 * 1024;

    context_3d_ = sgl_make_context(&context_desc_3d);

    sg_pipeline_desc pip_3d_default_desc    = {0};
    pip_3d_default_desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    pip_3d_default_desc.depth.write_enabled = true;
    pip_3d_default_                         = sgl_context_make_pipeline(context_3d_, &pip_3d_default_desc);

    sg_pipeline_desc pip_3d_add_desc               = {0};
    pip_3d_add_desc.depth.compare                  = SG_COMPAREFUNC_LESS_EQUAL;
    pip_3d_add_desc.depth.write_enabled            = true;
    pip_3d_add_desc.colors[0].blend.enabled        = true;
    pip_3d_add_desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    pip_3d_add_desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE;
    //pip_3d_add_desc.colors[0].write_mask = SG_COLORMASK_RGB;
    pip_3d_add_                                    = sgl_context_make_pipeline(context_3d_, &pip_3d_add_desc);

    sg_pipeline_desc pip_3d_alpha_desc               = {0};
    pip_3d_alpha_desc.depth.compare                  = SG_COMPAREFUNC_LESS_EQUAL;
    pip_3d_alpha_desc.depth.write_enabled            = true;
    pip_3d_alpha_desc.colors[0].blend.enabled          = true;
    pip_3d_alpha_desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    pip_3d_alpha_desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    //pip_3d_alpha_desc.colors[0].write_mask = SG_COLORMASK_RGB;
    
    pip_3d_alpha_                                    = sgl_context_make_pipeline(context_3d_, &pip_3d_alpha_desc);

    // IMGUI
    simgui_desc_t imgui_desc = {0};
    imgui_desc.logger.func   = slog_func;
    simgui_setup(&imgui_desc);

    const sgimgui_desc_t sgimgui_desc = {0};
    sgimgui_init(&sgimgui_, &sgimgui_desc);
}

void SokolRenderState::StartFrame(void)
{
    int w, h;
    SDL_GL_GetDrawableSize(program_window, &w, &h);

    sg_pass_action pass_action;
    pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass_action.colors[0].clear_value = {0.0f, 0.0f, 0.0f, 1.0f};

    pass_action.depth.load_action = SG_LOADACTION_CLEAR;
    pass_action.depth.clear_value = 1.0f;

    sg_pass pass                  = {0};
    pass.action                   = pass_action;
    pass.swapchain.width          = w;
    pass.swapchain.height         = h;
    pass.swapchain.color_format   = SG_PIXELFORMAT_RGBA8;
    pass.swapchain.depth_format   = SG_PIXELFORMAT_DEPTH;
    pass.swapchain.gl.framebuffer = 0;

#ifdef SOKOL_D3D11
    pass.swapchain.d3d11.render_view = sapp_d3d11_get_render_view();
    pass.swapchain.d3d11.resolve_view = sapp_d3d11_get_resolve_view();
    pass.swapchain.d3d11.depth_stencil_view = sapp_d3d11_get_depth_stencil_view();
#endif

    sg_begin_pass(&pass);
}

void SokolRenderState::SwapBuffers(void)
{
    sapp_d3d11_present(false);
}

void SokolRenderState::FinishFrame(void)
{
    int w, h;
    SDL_GL_GetDrawableSize(program_window, &w, &h);

    simgui_frame_desc_t frame_desc = {0};
    frame_desc.width               = w;
    frame_desc.height              = h;
    frame_desc.delta_time          = 100;
    frame_desc.dpi_scale           = 1;

    sgimgui_.caps_window.open        = false;
    sgimgui_.buffer_window.open      = false;
    sgimgui_.pipeline_window.open    = false;
    sgimgui_.attachments_window.open = false;
    sgimgui_.frame_stats_window.open = false;

    simgui_new_frame(&frame_desc);
    sgimgui_draw(&sgimgui_);

    sgl_context_draw(context_3d_);
    sgl_context_draw(context_2d_);
    simgui_render();

    sg_end_pass();
    sg_commit();    
}
