#include "i_defs.h"
#include "i_defs_gl.h"

#include <vector>
#include <unordered_map>
#include <algorithm>

#include "image_data.h"

#include "dm_state.h"
#include "e_player.h"
#include "m_argv.h"
#include "r_gldefs.h"
#include "r_units.h"

#include "r_misc.h"
#include "r_image.h"
#include "r_texgl.h"
#include "r_shader.h"
#include "r_sky.h"

#include "r_colormap.h"

#include "AlmostEquals.h"
#include "edge_profiling.h"
#include "sokol_gfx.h"
#include "shaders/world.h"
#include "HandmadeMath.h"
#include "gfx.h"

// TODO review if these should be archived
DEF_CVAR(r_colorlighting, "1", 0)
DEF_CVAR(r_colormaterial, "1", 0)

#ifdef APPLE_SILICON
#define DUMB_CLAMP "1"
#else
#define DUMB_CLAMP "0"
#endif

DEF_CVAR(r_dumbsky, "0", 0)
DEF_CVAR(r_dumbmulti, "0", 0)
DEF_CVAR(r_dumbcombine, "0", 0)
DEF_CVAR(r_dumbclamp, DUMB_CLAMP, 0)

rgbcol_t current_fog_rgb = RGB_NO_VALUE;
GLfloat  current_fog_color[4];
float    current_fog_density = 0;
GLfloat  cull_fog_color[4];

#define DUMMY_CLAMP 789

extern cvar_c r_culling;
extern cvar_c r_cullfog;

std::unordered_map<GLuint, GLint> texture_clamp;

static struct
{
    sg_shader shd_world;
    sg_buffer vertex_buffer;
} state;

// a single vertex to pass to the GL
struct frame_vert_t
{
    uint8_t rgba[4];
    vec3_t  pos;
    vec2_t  texc[2];
};

#define MAX_FRAME_VERTS 524288
static frame_vert_t *frame_vertices = nullptr;
static int           cur_frame_vert = 0;

typedef struct local_gl_unit_s
{
    // unit mode (e.g. GL_TRIANGLE_FAN)
    GLuint shape;

    // environment modes (GL_REPLACE, GL_MODULATE, GL_DECAL, GL_ADD)
    GLuint env[2];

    // texture(s) used
    GLuint tex[2];

    // pass number (multiple pass rendering)
    int pass;

    // blending flags
    int blending;

    // range of local vertices
    int first, count;

    rgbcol_t fog_color   = RGB_NO_VALUE;
    float    fog_density = 0;
} local_gl_unit_t;

struct draw_command_t
{
    sg_pipeline pipeline;
    uint32_t    images[2];
    uint32_t    samplers[2];
    int         vert_first;
    int         vert_count;
};

#define MAX_DRAW_COMMANDS 65536
static draw_command_t *draw_commands = nullptr;
static int             cur_command   = 0;

#define MAX_L_UNIT 1024
static local_gl_unit_t local_units[MAX_L_UNIT];
static int             cur_unit;

#define MAX_L_VERT 65536
static local_gl_vert_t local_verts[MAX_L_VERT];
static int             cur_local_vert;

static std::unordered_set<uint32_t> frame_pipelines;

// RGL_InitUnits
//
// Initialise the unit system.  Once-only call.
//
void RGL_InitUnits(void)
{
    // Run the soft init code
    RGL_SoftInitUnits();

    draw_commands = (draw_command_t *)malloc(sizeof(draw_command_t) * MAX_DRAW_COMMANDS);

    sg_shader shd   = sg_make_shader(world_shader_desc(sg_query_backend()));
    state.shd_world = shd;

    frame_vertices = (frame_vert_t *)malloc(sizeof(frame_vert_t) * MAX_FRAME_VERTS);

    sg_buffer_desc buffer_desc = {0};
    buffer_desc.type           = SG_BUFFERTYPE_VERTEXBUFFER;
    buffer_desc.size           = sizeof(frame_vert_t) * MAX_FRAME_VERTS;
    buffer_desc.usage          = SG_USAGE_STREAM;
    buffer_desc.label          = "r_units-vertices";

    state.vertex_buffer = sg_make_buffer(&buffer_desc);
}

//
// RGL_SoftInitUnits
//
// -ACB- 2004/02/15 Quickly-hacked routine to reinit stuff lost on res change
//
void RGL_SoftInitUnits()
{
}

void GFX_Frame()
{
    cur_frame_vert = 0;
    cur_command    = 0;
    frame_pipelines.clear();
}

#pragma pack(push, 1)
SOKOL_SHDC_ALIGN(16) typedef struct my_vs_params_t
{
    HMM_Mat4 mvp;
} my_vs_params_t;
#pragma pack(pop)

extern angle_t viewangle;
extern angle_t viewvertangle;
extern float   viewx;
extern float   viewy;
extern float   viewz;

void GFX_DrawWorld()
{

    if (!cur_command)
    {
        return;
    }

    my_vs_params_t vs_params;

    HMM_Mat4 proj = HMM_Perspective_RH_ZO(HMM_AngleDeg(75.0f), 1920.0f / 1080.0f, r_nearclip.f, r_farclip.f);

    HMM_Mat4 view = HMM_M4D(1.0f);
    view = HMM_Mul(view, HMM_Rotate_RH(HMM_AngleDeg(270.0f - ANG_2_FLOAT(viewvertangle)), HMM_V3(1.0f, 0.0f, 0.0f)));
    view = HMM_Mul(view, HMM_Rotate_RH(HMM_AngleDeg(90.0f - ANG_2_FLOAT(viewangle)), HMM_V3(0.0f, 0.0f, 1.0f)));

    const float t = 1.0f;
    view          = HMM_Mul(view, HMM_Translate(HMM_V3(-viewx * t, -viewy * t, -viewz * t)));

    HMM_Mat4 view_proj = HMM_Mul(proj, view);

    vs_params.mvp = view_proj;

    sg_range range;
    range.ptr  = frame_vertices;
    range.size = sizeof(frame_vert_t) * cur_frame_vert;

    sg_update_buffer(state.vertex_buffer, range);

    for (auto pip : frame_pipelines)
    {
        bool     applied  = false;
        uint32_t cimage   = 0;
        uint32_t csampler = 0;

        for (int i = 0; i < cur_command; i++)
        {
            draw_command_t *cmd = &draw_commands[i];

            if (cmd->pipeline.id != pip)
            {
                continue;
            }

            if (!cmd->vert_count)
            {
                continue;
            }

            bool need_bind = false;

            if (!applied)
            {
                applied = true;
                sg_apply_pipeline(cmd->pipeline);

                // OPTIMIZE: apply per shader pipeling first?
                range = SG_RANGE(vs_params);
                sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &range);

                need_bind = true;
            }
            else
            {
                need_bind = cimage != cmd->images[0] || csampler != cmd->samplers[0];
            }

            if (need_bind)
            {
                sg_bindings bind       = {0};
                bind.vertex_buffers[0] = state.vertex_buffer;
                bind.fs.images[0].id   = cmd->images[0];
                bind.fs.images[1].id   = cmd->images[1];
                bind.fs.samplers[0].id = cmd->samplers[0];
                bind.fs.samplers[1].id = cmd->samplers[1];
                sg_apply_bindings(&bind);
            }

            cimage   = cmd->images[0];
            csampler = cmd->samplers[0];

            sg_draw(cmd->vert_first, cmd->vert_count, 1);
        }
    }

    glUseProgram(0);
    // may need to unbind other samples
    glBindSampler(0, 0);
}

void RGL_StartUnits(bool sort_em)
{
    cur_local_vert = cur_unit = 0;
}

void RGL_FinishUnits(void)
{
    RGL_DrawUnits();
}

local_gl_vert_t *RGL_BeginUnit(GLuint shape, int max_vert, GLuint env1, GLuint tex1, GLuint env2, GLuint tex2, int pass,
                               int blending, rgbcol_t fog_color, float fog_density)

{
    SYS_ASSERT(max_vert > 0);
    SYS_ASSERT(pass >= 0);

    if (cur_local_vert + max_vert >= MAX_L_VERT || cur_unit >= MAX_L_UNIT)
    {
        RGL_DrawUnits();
    }

    local_gl_unit_t *unit = local_units + cur_unit;
    if (env1 == ENV_NONE)
        tex1 = 0;
    if (env2 == ENV_NONE)
        tex2 = 0;

    unit->shape  = shape;
    unit->env[0] = env1;
    unit->env[1] = env2;
    unit->tex[0] = tex1;
    unit->tex[1] = tex2;

    unit->pass     = pass;
    unit->blending = blending;
    unit->first    = cur_local_vert; // count set later

    unit->fog_color   = fog_color;
    unit->fog_density = fog_density;

    return local_verts + cur_local_vert;
}

void RGL_EndUnit(int actual_vert)
{

    local_gl_unit_t *unit = local_units + cur_unit;
    unit->count           = actual_vert;
    cur_unit++;
    cur_local_vert += actual_vert;

    SYS_ASSERT(cur_local_vert <= MAX_L_VERT);
    SYS_ASSERT(cur_unit <= MAX_L_UNIT);
}

extern int hack_buffer_update;

#define BUCKET_MAX_VERTICES 65535
#define BUCKET_MAX_INDICES  BUCKET_MAX_VERTICES / 3

/*
typedef enum
{
    BL_NONE = 0,

    BL_Masked = (1 << 0), // drop fragments when alpha == 0
    BL_Less   = (1 << 1), // drop fragments when alpha < color.a
    BL_Alpha  = (1 << 2), // alpha-blend with the framebuffer
    BL_Add    = (1 << 3), // additive-blend with the framebuffer

    BL_CullBack  = (1 << 4), // enable back-face culling
    BL_CullFront = (1 << 5), // enable front-face culling
    BL_NoZBuf    = (1 << 6), // don't update the Z buffer
    BL_ClampY    = (1 << 7), // force texture to be Y clamped
} blending_mode_e;

#define BL_CULL_BOTH (BL_CullBack | BL_CullFront)

*/

#define MAX_PASSES 12

static std::unordered_map<int, sg_pipeline> runit_pipelines[MAX_PASSES];

static int pipeline_count = 0;

static inline sg_pipeline GFX_GetDrawUnitPipeline(local_gl_unit_t *unit)
{
    // OPTIMIZE:
    // Do we need polygon offset, or can we disable depth compare, end up with a lot of passes

    SYS_ASSERT(unit->pass < MAX_PASSES);
    std::unordered_map<int, sg_pipeline> &blend_lookup = runit_pipelines[unit->pass];

    // mask out blends at shader uniform level
    int pip_blend = unit->blending;
    pip_blend &= ~BL_Masked;
    pip_blend &= ~BL_Less;

    auto search = blend_lookup.find(pip_blend);
    if (search != blend_lookup.end())
    {
        return search->second;
    }

    // depth
    float depth_bias          = unit->pass ? -unit->pass : 0.0f;
    bool  depth_write_enabled = unit->blending & BL_NoZBuf ? false : true;

    // culling mode
    sg_cull_mode cull_mode = SG_CULLMODE_NONE;
    if (unit->blending & BL_CULL_BOTH)
    {
        cull_mode = unit->blending & BL_CullFront ? SG_CULLMODE_FRONT : SG_CULLMODE_BACK;
    }

    sg_pipeline_desc desc = {0};
    desc.shader           = state.shd_world;
    desc.cull_mode        = cull_mode;

    desc.depth.write_enabled = depth_write_enabled;
    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.bias          = depth_bias;

    desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;

    desc.layout.buffers[0].stride = sizeof(frame_vert_t);

    /*
    sg_vertex_attr_state *normal = &desc.layout.attrs[3];
    normal->offset               = offsetof(local_gl_vert_t, normal);
    normal->format               = SG_VERTEXFORMAT_FLOAT3;
    */

    desc.primitive_type                      = SG_PRIMITIVETYPE_TRIANGLES;
    desc.layout.attrs[ATTR_vs_color0].format = SG_VERTEXFORMAT_UBYTE4N;
    desc.layout.attrs[ATTR_vs_color0].offset = offsetof(frame_vert_t, rgba);

    desc.layout.attrs[ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_vs_position].offset = offsetof(frame_vert_t, pos);

    desc.layout.attrs[ATTR_vs_texcoords].format = SG_VERTEXFORMAT_FLOAT4;
    desc.layout.attrs[ATTR_vs_texcoords].offset = offsetof(frame_vert_t, texc);
    /*
    desc.layout.attrs[ATTR_vs_normal].format    = SG_VERTEXFORMAT_FLOAT3;
    */

    sg_pipeline pip = sg_make_pipeline(&desc);

    blend_lookup.insert({pip_blend, pip});

    I_Printf("Pipelines: %i\n", ++pipeline_count);

    return pip;
}

void RGL_DrawUnits(void)
{

    if (cur_unit == 0)
        return;

    draw_command_t *cmd = nullptr;

    for (int i = 0; i < cur_unit; i++)
    {
        local_gl_unit_t *unit = &local_units[i];

        // this causes black mid polygons
        if (unit->blending & BL_ClampY)
        {
            continue;
        }

        if (unit->pass > 0)
        {
            continue;
        }

        if (unit->shape != GL_POLYGON || unit->count < 3)
        {
            continue;
        }

        sg_pipeline pip = GFX_GetDrawUnitPipeline(unit);

        uint32_t images[2]   = {0};
        uint32_t samplers[2] = {0};

        frame_pipelines.insert(pip.id);

        if (unit->tex[0])
        {
            auto search = gfx_image_lookup.find(unit->tex[0]);
            SYS_ASSERT(search != gfx_image_lookup.end());
            images[0]   = search->second.image_id;
            samplers[0] = search->second.sampler_id;
        }

        if (!cmd || cmd->pipeline.id != pip.id || cmd->images[0] != images[0] || cmd->images[1] != images[1] ||
            cmd->samplers[0] != samplers[0] || cmd->samplers[1] != samplers[1])
        {
            cmd              = &draw_commands[cur_command++];
            cmd->vert_first  = cur_frame_vert;
            cmd->vert_count  = 0;
            cmd->pipeline    = pip;
            cmd->images[0]   = images[0];
            cmd->images[1]   = images[1];
            cmd->samplers[0] = samplers[0];
            cmd->samplers[1] = samplers[1];
        }
        if (unit->shape == GL_POLYGON)
        {
            local_gl_vert_t *local = local_verts + unit->first;
            for (int j = 0; j < unit->count - 1; j++)
            {
                frame_vert_t *dest = frame_vertices + cur_frame_vert;

                local_gl_vert_t *src = &local[0];
                dest->pos            = src->pos;
                dest->texc[0]        = src->texc[0];
                dest->texc[1]        = src->texc[1];
                dest->rgba[0]        = uint8_t(src->rgba[0] * 255.0f);
                dest->rgba[1]        = uint8_t(src->rgba[1] * 255.0f);
                dest->rgba[2]        = uint8_t(src->rgba[2] * 255.0f);
                dest->rgba[3]        = uint8_t(src->rgba[3] * 255.0f);
                dest++;
                src           = &local[j + 1];
                dest->pos     = src->pos;
                dest->texc[0] = src->texc[0];
                dest->texc[1] = src->texc[1];
                dest->rgba[0] = uint8_t(src->rgba[0] * 255.0f);
                dest->rgba[1] = uint8_t(src->rgba[1] * 255.0f);
                dest->rgba[2] = uint8_t(src->rgba[2] * 255.0f);
                dest->rgba[3] = uint8_t(src->rgba[3] * 255.0f);
                dest++;
                src           = &local[(j + 2) % unit->count];
                dest->pos     = src->pos;
                dest->texc[0] = src->texc[0];
                dest->texc[1] = src->texc[1];
                dest->rgba[0] = uint8_t(src->rgba[0] * 255.0f);
                dest->rgba[1] = uint8_t(src->rgba[1] * 255.0f);
                dest->rgba[2] = uint8_t(src->rgba[2] * 255.0f);
                dest->rgba[3] = uint8_t(src->rgba[3] * 255.0f);
                dest++;

                cur_frame_vert += 3;
                cmd->vert_count += 3;
            }

            if (cur_frame_vert >= MAX_FRAME_VERTS - 3)
            {
                I_Error("Ran out of frame verts %i", cur_frame_vert);
            }
        }

        SYS_ASSERT(cur_command < MAX_DRAW_COMMANDS);
    }

    cur_local_vert = cur_unit = 0;
}
