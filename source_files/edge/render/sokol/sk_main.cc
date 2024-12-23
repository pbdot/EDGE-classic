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

extern bool            custom_skybox;
extern int             maximum_texture_size;
extern SkyStretch      current_sky_stretch;

static struct
{
    sg_pass_action pass_action;
    sg_pipeline    pip;
    sg_image       img;
    sg_bindings    bind;
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
    global_render_state->SetRenderMode(kRenderMode2D);

    sgl_viewport(0, 0, current_screen_width, current_screen_height, false);

    sgl_matrix_mode_projection();
    sgl_load_identity();
    sgl_ortho(0.0f, (float)current_screen_width, 0.0f, (float)current_screen_height, -1.0f, 1.0f);

    sgl_matrix_mode_modelview();
    sgl_load_identity();
}

//
// SetupWorldMatrices2D
//
// Setup the GL matrices for drawing 2D stuff within the "world" rendered by
// HUDRenderWorld
//
void SetupWorldMatrices2D(void)
{
    global_render_state->SetRenderMode(kRenderMode2D);

    sgl_viewport(view_window_x, view_window_y, view_window_width, view_window_height, false);

    sgl_matrix_mode_projection();
    sgl_load_identity();
    sgl_ortho((float)view_window_x, (float)view_window_width, (float)view_window_y, (float)view_window_height,
                -1.0f, 1.0f);

    sgl_matrix_mode_modelview();
    sgl_load_identity();
}

//
// SetupMatrices3d
//
// Setup the GL matrices for drawing 3D stuff.
//
void SetupMatrices3d(void)
{
    global_render_state->SetRenderMode(kRenderMode3D);

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

void SetupSkyMatrices(void)
{
    global_render_state->SetRenderMode(kRenderMode3D);

    if (custom_skybox)
    {
        sgl_matrix_mode_projection();
        sgl_push_matrix();
        sgl_load_identity();

        sgl_frustum(view_x_slope * renderer_near_clip.f_, -view_x_slope * renderer_near_clip.f_,
                    -view_y_slope * renderer_near_clip.f_, view_y_slope * renderer_near_clip.f_,
                    renderer_near_clip.f_, renderer_far_clip.f_);

        sgl_matrix_mode_modelview();
        sgl_push_matrix();
        sgl_load_identity();

        sgl_rotate(sgl_rad(270.0f) - epi::RadiansFromBAM(view_vertical_angle), 1.0f, 0.0f, 0.0f);
        sgl_rotate(epi::RadiansFromBAM(view_angle), 0.0f, 0.0f, 1.0f);
    }
    else
    {
        sgl_matrix_mode_projection();
        sgl_push_matrix();
        sgl_load_identity();

        sgl_frustum(-view_x_slope * renderer_near_clip.f_, view_x_slope * renderer_near_clip.f_,
                    -view_y_slope * renderer_near_clip.f_, view_y_slope * renderer_near_clip.f_,
                    renderer_near_clip.f_, renderer_far_clip.f_ * 4.0);

        sgl_matrix_mode_modelview();
        sgl_push_matrix();
        sgl_load_identity();

        sgl_rotate(sgl_rad(270.0f) - epi::RadiansFromBAM(view_vertical_angle), 1.0f, 0.0f, 0.0f);
        sgl_rotate(-epi::RadiansFromBAM(view_angle), 0.0f, 0.0f, 1.0f);

        if (current_sky_stretch == kSkyStretchStretch)
            sgl_translate(0.0f, 0.0f,
                          (renderer_far_clip.f_ * 2 * 0.15));  // Draw center above horizon a little
        else
            sgl_translate(0.0f, 0.0f,
                          -(renderer_far_clip.f_ * 2 * 0.15)); // Draw center below horizon a little
    }
}

void RendererRevertSkyMatrices(void)
{
    sgl_matrix_mode_projection();
    sgl_pop_matrix();

    sgl_matrix_mode_modelview();
    sgl_pop_matrix();
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
    global_render_state->Disable(GL_POLYGON_SMOOTH);

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
    global_render_state->Initialize();
    AllocateDrawStructs();

    maximum_texture_size = 4096;
}
