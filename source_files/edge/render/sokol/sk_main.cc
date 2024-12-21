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
#include "sk_state.h"

extern int             maximum_texture_size;
extern ConsoleVariable fliplevels;

static struct
{
    sg_pass_action pass_action;
    sg_pipeline    pip;
    sg_image       img;
    sg_bindings    bind;
    int            width_height;
    bool           immutable;
    sgimgui_t      sgimgui;
} sk_state = {};

//
// SetupMatrices2D
//
// Setup the GL matrices for drawing 2D stuff.
//
void SetupMatrices2D(void)
{
}

//
// SetupWorldMatrices2D
//
// Setup the GL matrices for drawing 2D stuff within the "world" rendered by
// HUDRenderWorld
//
void SetupWorldMatrices2D(void)
{
}

//
// SetupMatrices3d
//
// Setup the GL matrices for drawing 3D stuff.
//
void SetupMatrices3d(void)
{
}

static inline const char *SafeStr(const void *s)
{
    return s ? (const char *)s : "";
}

//
// RendererSoftInit
//
// All the stuff that can be re-initialised multiple times.
//
void RendererSoftInit(void)
{
}


static sg_environment edge_environment(void) {
    sg_environment env;
    memset(&env, sizeof(env), 0);    
    env.defaults.color_format = SG_PIXELFORMAT_RGBA8;
    env.defaults.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    env.defaults.sample_count = 1;
    return env;
}

//
// RendererInit
//
void RendererInit(void)
{
    memset(&sk_state, sizeof(sk_state), 0);
    sk_state.width_height = 16;
    sk_state.immutable    = false;

    sg_desc desc{0};
    desc.environment = edge_environment();
    desc.logger.func     = slog_func;
    desc.image_pool_size = 8192;
    sg_setup(&desc);

    if (!sg_isvalid())
    {
        FatalError("Sokol invalid");
    }

    simgui_desc_t imgui_desc = {0};
    imgui_desc.logger.func   = slog_func;
    simgui_setup(&imgui_desc);

    const sgimgui_desc_t sgimgui_desc = {0};
    sgimgui_init(&sk_state.sgimgui, &sgimgui_desc);

    sk_state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    sk_state.pass_action.colors[0].clear_value = {0.0f, 0.0f, 1.0f, 1.0f};

    AllocateDrawStructs();
}

void SokolRenderState::StartFrame(void)
{
    int w, h;
    SDL_GL_GetDrawableSize(program_window, &w, &h);

    sg_pass pass          = {0};
    pass.action           = sk_state.pass_action;
    pass.swapchain.width  = w;
    pass.swapchain.height = h;
    pass.swapchain.color_format = SG_PIXELFORMAT_RGBA8;
    pass.swapchain.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    pass.swapchain.gl.framebuffer = 0;

    sg_begin_pass(&pass);

    simgui_frame_desc_t frame_desc = {0};
    frame_desc.width               = w;
    frame_desc.height              = h;
    frame_desc.delta_time          = 100;
    frame_desc.dpi_scale           = 1;

    sk_state.sgimgui.caps_window.open = true;

    simgui_new_frame(&frame_desc);
    sgimgui_draw(&sk_state.sgimgui);
    simgui_render();
}

void SokolRenderState::SwapBuffers(void)
{
}

void SokolRenderState::FinishFrame(void)
{
    sg_end_pass();
    sg_commit();
}
