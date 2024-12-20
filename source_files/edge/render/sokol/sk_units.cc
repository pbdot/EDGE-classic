
#include <algorithm>
#include <unordered_map>
#include <vector>

#include "AlmostEquals.h"
#include "dm_state.h"
#include "e_player.h"
#include "edge_profiling.h"
#include "epi.h"
#include "i_defs_gl.h"
#include "im_data.h"
#include "m_argv.h"
#include "r_colormap.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_shader.h"
#include "r_sky.h"
#include "r_texgl.h"
#include "r_units.h"
#include "sk_glstubs.h"
#include "sk_local.h"
#include "sk_state.h"

EDGE_DEFINE_CONSOLE_VARIABLE(renderer_dumb_sky, "0", kConsoleVariableFlagArchive)
#ifdef APPLE_SILICON
EDGE_DEFINE_CONSOLE_VARIABLE(renderer_dumb_clamp, "1", kConsoleVariableFlagNone)
#else
EDGE_DEFINE_CONSOLE_VARIABLE(renderer_dumb_clamp, "0", kConsoleVariableFlagNone)
#endif

static constexpr uint16_t kMaximumLocalUnits = 1024;

extern ConsoleVariable draw_culling;
extern ConsoleVariable cull_fog_color;

std::unordered_map<GLuint, GLint> texture_clamp_s;
std::unordered_map<GLuint, GLint> texture_clamp_t;

// a single unit (polygon, quad, etc) to pass to the GL
struct RendererUnit
{
    // unit mode (e.g. GL_TRIANGLE_FAN)
    GLuint shape;

    // environment modes (GL_REPLACE, GL_MODULATE, GL_DECAL, GL_ADD)
    GLuint environment_mode[2];

    // texture(s) used
    GLuint texture[2];

    // pass number (multiple pass rendering)
    int pass;

    // blending flags
    int blending;

    // range of local vertices
    int first, count;

    RGBAColor fog_color   = kRGBANoValue;
    float     fog_density = 0;
};

static RendererVertex local_verts[kMaximumLocalVertices];
static RendererUnit   local_units[kMaximumLocalUnits];

static std::vector<RendererUnit *> local_unit_map;

static int current_render_vert;
static int current_render_unit;

static bool batch_sort;

RGBAColor culling_fog_color;

//
// StartUnitBatch
//
// Starts a fresh batch of units.
//
// When 'sort_em' is true, the units will be sorted to keep
// texture changes to a minimum.  Otherwise, the batch is
// drawn in the same order as given.
//
void StartUnitBatch(bool sort_em)
{
    current_render_vert = current_render_unit = 0;

    batch_sort = sort_em;

    local_unit_map.resize(kMaximumLocalUnits);
}

//
// FinishUnitBatch
//
// Finishes a batch of units, drawing any that haven't been drawn yet.
//
void FinishUnitBatch(void)
{
    RenderCurrentUnits();
}

//
// BeginRenderUnit
//
// Begin a new unit, with the given parameters (mode and texture ID).
// `max_vert' is the maximum expected vertices of the quad/poly (the
// actual number can be less, but never more).  Returns a pointer to
// the first vertex structure.  `masked' should be true if the texture
// contains "holes" (like sprites).  `blended' should be true if the
// texture should be blended (like for translucent water or sprites).
//
RendererVertex *BeginRenderUnit(GLuint shape, int max_vert, GLuint env1, GLuint tex1, GLuint env2, GLuint tex2,
                                int pass, int blending, RGBAColor fog_color, float fog_density)
{
    RendererUnit *unit;

    EPI_ASSERT(max_vert > 0);
    EPI_ASSERT(pass >= 0);

    EPI_ASSERT((blending & (kBlendingCullBack | kBlendingCullFront)) != (kBlendingCullBack | kBlendingCullFront));

    // check we have enough space left
    if (current_render_vert + max_vert > kMaximumLocalVertices || current_render_unit >= kMaximumLocalUnits)
    {
        RenderCurrentUnits();
    }

    unit = local_units + current_render_unit;

    if (env1 == kTextureEnvironmentDisable)
        tex1 = 0;
    if (env2 == kTextureEnvironmentDisable)
        tex2 = 0;

    unit->shape               = shape;
    unit->environment_mode[0] = env1;
    unit->environment_mode[1] = env2;
    unit->texture[0]          = tex1;
    unit->texture[1]          = tex2;

    unit->pass     = pass;
    unit->blending = blending;
    unit->first    = current_render_vert; // count set later

    unit->fog_color   = fog_color;
    unit->fog_density = fog_density;

    return local_verts + current_render_vert;
}

//
// EndRenderUnit
//
void EndRenderUnit(int actual_vert)
{
    RendererUnit *unit;

    EPI_ASSERT(actual_vert >= 0);

    if (actual_vert == 0)
        return;

    unit = local_units + current_render_unit;

    unit->count = actual_vert;

    current_render_vert += actual_vert;
    current_render_unit++;

    EPI_ASSERT(current_render_vert <= kMaximumLocalVertices);
    EPI_ASSERT(current_render_unit <= kMaximumLocalUnits);
}

struct Compare_Unit_pred
{
    inline bool operator()(const RendererUnit *A, const RendererUnit *B) const
    {
        if (A->pass != B->pass)
            return A->pass < B->pass;

        if (A->texture[0] != B->texture[0])
            return A->texture[0] < B->texture[0];

        if (A->texture[1] != B->texture[1])
            return A->texture[1] < B->texture[1];

        if (A->environment_mode[0] != B->environment_mode[0])
            return A->environment_mode[0] < B->environment_mode[0];

        if (A->environment_mode[1] != B->environment_mode[1])
            return A->environment_mode[1] < B->environment_mode[1];

        return A->blending < B->blending;
    }
};

//
// RenderCurrentUnits
//
// Forces the set of current units to be drawn.  This call is
// optional (it never _needs_ to be called by client code).
//
void RenderCurrentUnits(void)
{
    EDGE_ZoneScoped;

    SokolRenderState *fixme = (SokolRenderState *)global_render_state;

    if (current_render_unit == 0)
        return;

    for (int i = 0; i < current_render_unit; i++)
        local_unit_map[i] = &local_units[i];

    if (batch_sort)
    {
        std::sort(local_unit_map.begin(), local_unit_map.begin() + current_render_unit, Compare_Unit_pred());
    }

    if (draw_culling.d_)
    {
        RGBAColor fogColor;
        switch (cull_fog_color.d_)
        {
        case 0:
            fogColor = culling_fog_color;
            break;
        case 1:
            // Not pure white, but 1.0f felt like a little much - Dasho
            fogColor = kRGBASilver;
            break;
        case 2:
            fogColor = 0x404040FF; // Find a constant to call this
            break;
        case 3:
            fogColor = kRGBABlack;
            break;
        default:
            fogColor = culling_fog_color;
            break;
        }

        global_render_state->ClearColor(fogColor);
        global_render_state->FogMode(GL_LINEAR);
        global_render_state->FogColor(fogColor);
        global_render_state->FogStart(renderer_far_clip.f_ - 750.0f);
        global_render_state->FogEnd(renderer_far_clip.f_ - 250.0f);
        global_render_state->Enable(GL_FOG);
    }
    else
        global_render_state->FogMode(GL_EXP); // if needed

    for (int j = 0; j < current_render_unit; j++)
    {
        ec_frame_stats.draw_render_units++;

        RendererUnit *unit = local_unit_map[j];

        EPI_ASSERT(unit->count > 0);

        if (!draw_culling.d_ && unit->fog_color != kRGBANoValue && !(unit->blending & kBlendingNoFog))
        {
            float density = unit->fog_density;
            global_render_state->ClearColor(unit->fog_color);
            global_render_state->FogColor(unit->fog_color);
            global_render_state->FogDensity(std::log1p(density));
            if (!AlmostEquals(density, 0.0f))
                global_render_state->Enable(GL_FOG);
            else
                global_render_state->Disable(GL_FOG);
        }
        else if (!draw_culling.d_ || (unit->blending & kBlendingNoFog))
            global_render_state->Disable(GL_FOG);

        global_render_state->PolygonOffset(0, -unit->pass);

        if (unit->blending & kBlendingLess)
        {
            // Alpha function is updated below, because the alpha
            // value can change from unit to unit while the
            // kBlendingLess flag remains set.
            global_render_state->Enable(GL_ALPHA_TEST);
        }
        else if (unit->blending & kBlendingMasked)
        {
            global_render_state->Enable(GL_ALPHA_TEST);
            global_render_state->AlphaFunction(GL_GREATER, 0);
        }
        else if (unit->blending & kBlendingGEqual)
        {
            global_render_state->Enable(GL_ALPHA_TEST);
            global_render_state->AlphaFunction(GL_GEQUAL,
                                               1.0f - (epi::GetRGBAAlpha(local_verts[unit->first].rgba) / 255.0f));
        }
        else
            global_render_state->Disable(GL_ALPHA_TEST);

        if (unit->blending & kBlendingAdd)
        {
            global_render_state->Enable(GL_BLEND);
            global_render_state->BlendFunction(GL_SRC_ALPHA, GL_ONE);
        }
        else if (unit->blending & kBlendingAlpha)
        {
            global_render_state->Enable(GL_BLEND);
            global_render_state->BlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else if (unit->blending & kBlendingInvert)
        {
            global_render_state->Enable(GL_BLEND);
            global_render_state->BlendFunction(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
        }
        else if (unit->blending & kBlendingNegativeGamma)
        {
            global_render_state->Enable(GL_BLEND);
            global_render_state->BlendFunction(GL_ZERO, GL_SRC_COLOR);
        }
        else if (unit->blending & kBlendingPositiveGamma)
        {
            global_render_state->Enable(GL_BLEND);
            global_render_state->BlendFunction(GL_DST_COLOR, GL_ONE);
        }
        else
            global_render_state->Disable(GL_BLEND);

        if (unit->blending & (kBlendingCullBack | kBlendingCullFront))
        {
            global_render_state->Enable(GL_CULL_FACE);
            global_render_state->CullFace((unit->blending & kBlendingCullFront) ? GL_FRONT : GL_BACK);
        }
        else
            global_render_state->Disable(GL_CULL_FACE);

        global_render_state->DepthMask((unit->blending & kBlendingNoZBuffer) ? false : true);

        if (unit->blending & kBlendingLess)
        {
            // NOTE: assumes alpha is constant over whole polygon
            float a = epi::GetRGBAAlpha(local_verts[unit->first].rgba) / 255.0f;
            global_render_state->AlphaFunction(GL_GREATER, a * 0.66f);
        }

        GLint old_clamp_s = kDummyClamp;
        GLint old_clamp_t = kDummyClamp;

        uint32_t pipeline_flags = 0;
        if (fixme->depth_mask_)
            pipeline_flags |= kPipelineDepthWrite;
        if (fixme->depth_function_ == GL_GREATER)
            pipeline_flags |= kPipelineDepthGreater;
        if (unit->blending & kBlendingAlpha)
            pipeline_flags |= kPipelineAlpha;
        if (unit->blending & kBlendingAdd)
            pipeline_flags |= kPipelineAdditive;

        sgl_load_pipeline(fixme->GetPipeline(pipeline_flags));

        if (unit->texture[0] && unit->environment_mode[0] != kTextureEnvironmentDisable)
        {
            sgl_enable_texture();
            sg_image img0;
            img0.id = unit->texture[0];

            sg_sampler img0_sampler;
            img0_sampler.id = fixme->image_samplers_.find(img0.id)->second;

            if (!unit->texture[1] || unit->environment_mode[1] == kTextureEnvironmentDisable)
            {
                sgl_texture(img0, img0_sampler);
            }
            else
            {
                sg_image img1;
                img1.id = unit->texture[1];
                sg_sampler img1_sampler;
                img1_sampler.id = fixme->image_samplers_.find(img1.id)->second;

                sgl_multi_texture(img0, img0_sampler, img1, img1_sampler);
            }
        }
        else
        {
            sgl_disable_texture();
        }

        /*
        for (int t = 1; t >= 0; t--)
        {
            global_render_state->ActiveTexture(GL_TEXTURE0 + t);

            if (draw_culling.d_ && !(unit->blending & kBlendingNoFog))
            {
                if (unit->pass > 0)
                    global_render_state->Disable(GL_FOG);
                else
                    global_render_state->Enable(GL_FOG);
            }

            if (!unit->texture[t])
                global_render_state->Disable(GL_TEXTURE_2D);
            else
            {
                global_render_state->Enable(GL_TEXTURE_2D);
                global_render_state->BindTexture(unit->texture[t]);
            }

            if (!t && (unit->blending & kBlendingRepeatX) && unit->texture[0])
            {
                auto existing = texture_clamp_s.find(unit->texture[0]);
                if (existing != texture_clamp_s.end())
                {
                    if (existing->second != GL_REPEAT)
                    {
                        old_clamp_s = existing->second;
                        global_render_state->TextureWrapS(GL_REPEAT);
                    }
                }
                else
                    global_render_state->TextureWrapS(GL_REPEAT);
            }

            if (!t && (unit->blending & (kBlendingClampY | kBlendingRepeatY)) && unit->texture[0])
            {
                auto existing = texture_clamp_t.find(unit->texture[0]);
                if (existing != texture_clamp_t.end())
                {
                    if (unit->blending & kBlendingClampY)
                    {
                        if (existing->second != (renderer_dumb_clamp.d_ ? GL_CLAMP : GL_CLAMP_TO_EDGE))
                        {
                            old_clamp_t = existing->second;
                            global_render_state->TextureWrapT(renderer_dumb_clamp.d_ ? GL_CLAMP : GL_CLAMP_TO_EDGE);
                        }
                    }
                    else
                    {
                        if (existing->second != GL_REPEAT)
                        {
                            old_clamp_t = existing->second;
                            global_render_state->TextureWrapT(GL_REPEAT);
                        }
                    }
                }
                else
                {
                    if (unit->blending & kBlendingClampY)
                        global_render_state->TextureWrapT(renderer_dumb_clamp.d_ ? GL_CLAMP : GL_CLAMP_TO_EDGE);
                    else
                        global_render_state->TextureWrapT(GL_REPEAT);
                }
            }

            if (unit->environment_mode[t] == kTextureEnvironmentSkipRGB)
            {
                global_render_state->TextureEnvironmentMode(GL_COMBINE);
                global_render_state->TextureEnvironmentCombineRGB(GL_REPLACE);
                global_render_state->TextureEnvironmentSource0RGB(GL_PREVIOUS);
            }
            else
            {
                if (unit->environment_mode[t] != kTextureEnvironmentDisable)
                    global_render_state->TextureEnvironmentMode(unit->environment_mode[t]);
                global_render_state->TextureEnvironmentCombineRGB(GL_MODULATE);
                global_render_state->TextureEnvironmentSource0RGB(GL_TEXTURE);
            }
        }
        */

        float fogr = float(epi::GetRGBARed(fixme->fog_color_)) / 255.0f;
        float fogg = float(epi::GetRGBAGreen(fixme->fog_color_)) / 255.0f;
        float fogb = float(epi::GetRGBABlue(fixme->fog_color_)) / 255.0f;

        sgl_set_fog(fixme->enable_fog_, fogr, fogg, fogb, 1, std::log1p(fixme->fog_density_), fixme->fog_start_,
                    fixme->fog_end_, 1);

        // glBegin(unit->shape);
        if (unit->shape == GL_QUADS)
        {
            sgl_begin_quads();
        }
        else if (unit->shape == GL_TRIANGLES)
        {
            sgl_begin_triangles();
        }
        else if (unit->shape == GL_POLYGON)
        {
            // TODO: can be strips

            const RendererVertex *V = local_verts + unit->first;

            sgl_begin_triangles();

            for (int k = 0; k < unit->count - 1; k++)
            {
                const RendererVertex *V1 = &V[k + 1];
                const RendererVertex *V2 = &V[((k + 2) % unit->count)];

                sgl_v3f_t4f_c4b(V->position.X, V->position.Y, V->position.Z, V->texture_coordinates[0].X,
                                V->texture_coordinates[0].Y, V->texture_coordinates[1].X, V->texture_coordinates[1].Y,
                                epi::GetRGBARed(V->rgba), epi::GetRGBAGreen(V->rgba), epi::GetRGBABlue(V->rgba),
                                epi::GetRGBAAlpha(V->rgba));

                sgl_v3f_t4f_c4b(V1->position.X, V1->position.Y, V1->position.Z, V1->texture_coordinates[0].X,
                                V1->texture_coordinates[0].Y, V1->texture_coordinates[1].X,
                                V1->texture_coordinates[1].Y, epi::GetRGBARed(V1->rgba), epi::GetRGBAGreen(V1->rgba),
                                epi::GetRGBABlue(V1->rgba), epi::GetRGBAAlpha(V1->rgba));

                sgl_v3f_t4f_c4b(V2->position.X, V2->position.Y, V2->position.Z, V2->texture_coordinates[0].X,
                                V2->texture_coordinates[0].Y, V2->texture_coordinates[1].X,
                                V2->texture_coordinates[1].Y, epi::GetRGBARed(V2->rgba), epi::GetRGBAGreen(V2->rgba),
                                epi::GetRGBABlue(V2->rgba), epi::GetRGBAAlpha(V2->rgba));
            }

            sgl_end();
            continue;
        }
        else if (unit->shape == GL_LINES)
        {
            sgl_disable_texture();
            sgl_begin_lines();
            const RendererVertex *V = local_verts + unit->first;

            for (int v_idx = 0, v_last_idx = unit->count; v_idx < v_last_idx; v_idx++, V++)
            {
                sgl_v3f_c4b(V->position.X, V->position.Y, V->position.Z, epi::GetRGBARed(V->rgba),
                            epi::GetRGBAGreen(V->rgba), epi::GetRGBABlue(V->rgba), epi::GetRGBAAlpha(V->rgba));
            }

            sgl_end();

            continue;
        }

        else
        {
            continue;
        }

        const RendererVertex *V = local_verts + unit->first;

        for (int v_idx = 0, v_last_idx = unit->count; v_idx < v_last_idx; v_idx++, V++)
        {
            sgl_v3f_t4f_c4b(V->position.X, V->position.Y, V->position.Z, V->texture_coordinates[0].X,
                            V->texture_coordinates[0].Y, V->texture_coordinates[1].X, V->texture_coordinates[1].Y,
                            epi::GetRGBARed(V->rgba), epi::GetRGBAGreen(V->rgba), epi::GetRGBABlue(V->rgba),
                            epi::GetRGBAAlpha(V->rgba));
        }

        sgl_end();

        // restore the clamping mode
        if (old_clamp_s != kDummyClamp)
        {
            global_render_state->TextureWrapS(old_clamp_s);
        }
        if (old_clamp_t != kDummyClamp)
        {
            global_render_state->TextureWrapT(old_clamp_t);
        }
    }

    // all done
    current_render_vert = current_render_unit = 0;

    global_render_state->PolygonOffset(0, 0);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
