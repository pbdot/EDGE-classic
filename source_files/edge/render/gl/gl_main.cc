#include "epi_str_compare.h"
#include "g_game.h"
#include "i_defs_gl.h"
#include "r_colormap.h"
#include "r_draw.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_units.h"

// implementation limits

static int maximum_lights;
static int maximum_clip_planes;
static int maximum_texture_units;
extern int maximum_texture_size;
//
// SetupMatrices2D
//
// Setup the GL matrices for drawing 2D stuff.
//
void SetupMatrices2D(void)
{
    glViewport(0, 0, current_screen_width, current_screen_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, (float)current_screen_width, 0.0f, (float)current_screen_height, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

//
// SetupWorldMatrices2D
//
// Setup the GL matrices for drawing 2D stuff within the "world" rendered by
// HUDRenderWorld
//
void SetupWorldMatrices2D(void)
{
    glViewport(view_window_x, view_window_y, view_window_width, view_window_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho((float)view_window_x, (float)view_window_width, (float)view_window_y, (float)view_window_height, -1.0f,
            1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

//
// SetupMatrices3d
//
// Setup the GL matrices for drawing 3D stuff.
//
void SetupMatrices3d(void)
{
    glViewport(view_window_x, view_window_y, view_window_width, view_window_height);

    // calculate perspective matrix

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    glFrustum(-view_x_slope * renderer_near_clip.f_, view_x_slope * renderer_near_clip.f_,
                -view_y_slope * renderer_near_clip.f_, view_y_slope * renderer_near_clip.f_, renderer_near_clip.f_,
                renderer_far_clip.f_);

    // calculate look-at matrix

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    glRotatef(270.0f - epi::DegreesFromBAM(view_vertical_angle), 1.0f, 0.0f, 0.0f);
    glRotatef(90.0f - epi::DegreesFromBAM(view_angle), 0.0f, 0.0f, 1.0f);
    glTranslatef(-view_x, -view_y, -view_z);
}

static inline const char *SafeStr(const void *s)
{
    return s ? (const char *)s : "";
}

//
// RendererCheckExtensions
//
// Based on code by Bruce Lewis.
//
void RendererCheckExtensions(void)
{
    // -ACB- 2004/08/11 Made local: these are not yet used elsewhere
    std::string glstr_version(SafeStr(glGetString(GL_VERSION)));
    std::string glstr_renderer(SafeStr(glGetString(GL_RENDERER)));
    std::string glstr_vendor(SafeStr(glGetString(GL_VENDOR)));

    LogPrint("OpenGL: Version: %s\n", glstr_version.c_str());
    LogPrint("OpenGL: Renderer: %s\n", glstr_renderer.c_str());
    LogPrint("OpenGL: Vendor: %s\n", glstr_vendor.c_str());
}

//
// RendererSoftInit
//
// All the stuff that can be re-initialised multiple times.
//
void RendererSoftInit(void)
{
    global_render_state->Disable(GL_BLEND);
    global_render_state->Disable(GL_LIGHTING);
    global_render_state->Disable(GL_COLOR_MATERIAL);
    global_render_state->Disable(GL_CULL_FACE);
    global_render_state->Disable(GL_DEPTH_TEST);
    global_render_state->Disable(GL_SCISSOR_TEST);
    global_render_state->Disable(GL_STENCIL_TEST);

    global_render_state->Disable(GL_LINE_SMOOTH);

#ifndef EDGE_GL_ES2
    global_render_state->Disable(GL_POLYGON_SMOOTH);
#endif

    global_render_state->Enable(GL_NORMALIZE);

    global_render_state->ShadeModel(GL_SMOOTH);
    global_render_state->DepthFunction(GL_LEQUAL);
    global_render_state->AlphaFunction(GL_GREATER, 0);

    global_render_state->FrontFace(GL_CW);
    global_render_state->CullFace(GL_BACK);
    global_render_state->Disable(GL_CULL_FACE);

    global_render_state->Hint(GL_FOG_HINT, GL_NICEST);
    global_render_state->Hint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

//
// RendererInit
//
void RendererInit(void)
{
    LogPrint("OpenGL: Initialising...\n");

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);


    RendererCheckExtensions();

    // read implementation limits
    {
        GLint max_tex_size;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
        maximum_texture_size = max_tex_size;
    }

    LogPrint("OpenGL: Tex: %d\n", maximum_texture_size);

    RendererSoftInit();

    AllocateDrawStructs();

    SetupMatrices2D();
}