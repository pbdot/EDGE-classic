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
#include "shaders/world_single_texture.h"
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
    sg_shader shd_world_single_texture;
    sg_buffer vertex_buffer;
    sg_buffer index_buffer;
} state;

#define MAX_FRAME_VERTS 524288
static frame_vert_t *frame_vertices = nullptr;
static int           cur_frame_vert = 0;

#define MAX_FRAME_INDICES 524288
static uint32_t *frame_indices   = nullptr;
static int       cur_frame_index = 0;

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
    int         index_first;
    int         index_count;
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

    frame_vertices = (frame_vert_t *)malloc(sizeof(frame_vert_t) * MAX_FRAME_VERTS);
    frame_indices  = (uint32_t *)malloc(sizeof(uint32_t) * MAX_FRAME_INDICES);

    draw_commands = (draw_command_t *)malloc(sizeof(draw_command_t) * MAX_DRAW_COMMANDS);

    state.shd_world                = sg_make_shader(world_shader_desc(sg_query_backend()));
    state.shd_world_single_texture = sg_make_shader(world_single_texture_shader_desc(sg_query_backend()));

    // vertex buffer
    sg_buffer_desc buffer_desc = {0};
    buffer_desc.type           = SG_BUFFERTYPE_VERTEXBUFFER;
    buffer_desc.size           = sizeof(frame_vert_t) * MAX_FRAME_VERTS;
    buffer_desc.usage          = SG_USAGE_STREAM;
    buffer_desc.label          = "r_units-vertices";

    state.vertex_buffer = sg_make_buffer(&buffer_desc);

    // index buffer
    buffer_desc       = {0};
    buffer_desc.type  = SG_BUFFERTYPE_INDEXBUFFER;
    buffer_desc.size  = sizeof(uint32_t) * MAX_FRAME_INDICES;
    buffer_desc.usage = SG_USAGE_STREAM;
    buffer_desc.label = "r_units-indices";

    state.index_buffer = sg_make_buffer(&buffer_desc);
}

//
// RGL_SoftInitUnits
//
// -ACB- 2004/02/15 Quickly-hacked routine to reinit stuff lost on res change
//
void RGL_SoftInitUnits()
{
}

void GFX_StartFrame()
{
    cur_frame_vert  = 0;
    cur_frame_index = 0;
    cur_command     = 0;
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

HMM_Mat4 frame_projection;

void GFX_DrawUnits(sg_pass world_pass)
{
    if (!cur_command)
    {
        return;
    }

    my_vs_params_t vs_params;

    float left   = -view_x_slope * r_nearclip.f;
    float right  = view_x_slope * r_nearclip.f;
    float bottom = -view_y_slope * r_nearclip.f;
    float top    = view_y_slope * r_nearclip.f;
    float fnear  = r_nearclip.f;
    float ffar   = r_farclip.f;

    float A = (right + left) / (right - left);
    float B = (top + bottom) / (top - bottom);
    float C = (-(ffar + fnear)) / (ffar - fnear);
    float D = (-(2 * ffar * fnear)) / (ffar - fnear);

    HMM_Mat4 proj;

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

    frame_projection = proj;

    HMM_Mat4 view = HMM_M4D(1.0f);
    view = HMM_Mul(view, HMM_Rotate_RH(HMM_AngleDeg(270.0f - ANG_2_FLOAT(viewvertangle)), HMM_V3(1.0f, 0.0f, 0.0f)));
    view = HMM_Mul(view, HMM_Rotate_RH(HMM_AngleDeg(90.0f - ANG_2_FLOAT(viewangle)), HMM_V3(0.0f, 0.0f, 1.0f)));

    const float t = 1.0f;
    view          = HMM_Mul(view, HMM_Translate(HMM_V3(-viewx * t, -viewy * t, -viewz * t)));

    HMM_Mat4 view_proj = HMM_Mul(proj, view);

    vs_params.mvp = view_proj;

    sg_range range;

    // vertex update
    range.ptr  = frame_vertices;
    range.size = sizeof(frame_vert_t) * cur_frame_vert;
    sg_update_buffer(state.vertex_buffer, range);

    // index update
    range.ptr  = frame_indices;
    range.size = sizeof(uint32_t) * cur_frame_index;
    sg_update_buffer(state.index_buffer, range);

    //void GFX_DrawWorldDepth(const HMM_Mat4& projection, int indices, sg_buffer vbuf, sg_buffer ibuf);
    //GFX_DrawWorldDepth(view_proj, cur_frame_index, state.vertex_buffer, state.index_buffer);

    sg_pass_action pass_action        = {0};
    pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass_action.colors[0].clear_value = {0.0f, 0.3f, 0.0f, 1.0f};
    pass_action.depth.load_action     = SG_LOADACTION_CLEAR;
    pass_action.depth.clear_value     = 1.0f;

    sg_begin_pass(world_pass, pass_action);


    uint32_t cur_pipe_id = 0;
    uint32_t cimage[2]   = {0};
    uint32_t csampler[2] = {0};

    std::unordered_set<uint32_t> pip_uniforms;

    for (int i = 0; i < cur_command; i++)
    {
        draw_command_t *cmd = &draw_commands[i];

        if (!cmd->index_count)
        {
            continue;
        }

        bool need_bind = false;
        if (cur_pipe_id != cmd->pipeline.id)
        {
            need_bind = true;
            sg_apply_pipeline(cmd->pipeline);

            // OPTIMIZE: apply per shader pipeling first?
            range = SG_RANGE(vs_params);
            sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &range);
        }

        need_bind = need_bind || cimage[0] != cmd->images[0] || csampler[0] != cmd->samplers[0] ||
                    cimage[1] != cmd->images[1] || csampler[2] != cmd->samplers[2];

        if (need_bind)
        {
            sg_bindings bind       = {0};
            bind.vertex_buffers[0] = state.vertex_buffer;
            bind.index_buffer      = state.index_buffer;
            bind.fs.images[0].id   = cmd->images[0];
            bind.fs.images[1].id   = cmd->images[1];
            bind.fs.samplers[0].id = cmd->samplers[0];
            bind.fs.samplers[1].id = cmd->samplers[1];

            sg_apply_bindings(&bind);
        }

        cur_pipe_id = cmd->pipeline.id;
        cimage[0]   = cmd->images[0];
        csampler[0] = cmd->samplers[0];
        cimage[1]   = cmd->images[1];
        csampler[1] = cmd->samplers[1];

        sg_draw(cmd->index_first, cmd->index_count, 1);
    }

    sg_end_pass();

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

#define MAX_SHADERS 2
#define MAX_PASSES  24

static std::unordered_map<int, sg_pipeline> runit_pipelines[MAX_SHADERS][MAX_PASSES];

static int pipeline_count = 0;

static inline sg_pipeline GFX_GetDrawUnitPipeline(local_gl_unit_t *unit)
{
    // OPTIMIZE:
    // Do we need polygon offset, or can we disable depth compare, end up with a lot of passes

    int shader_index = unit->tex[1] ? 1 : 0;

    SYS_ASSERT(unit->pass < MAX_PASSES);
    std::unordered_map<int, sg_pipeline> &blend_lookup = runit_pipelines[shader_index][unit->pass];

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
    desc.shader           = shader_index ? state.shd_world : state.shd_world_single_texture;
    desc.cull_mode        = cull_mode;

    desc.depth.write_enabled = depth_write_enabled;
    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.bias          = depth_bias;

    /*
        typedef struct sg_blend_state {
            bool enabled;
            sg_blend_factor src_factor_rgb;
            sg_blend_factor dst_factor_rgb;
            sg_blend_op op_rgb;
            sg_blend_factor src_factor_alpha;
            sg_blend_factor dst_factor_alpha;
            sg_blend_op op_alpha;
        } sg_blend_state;
    */

    if (pip_blend & BL_Add)
    {
        desc.colors[0].blend.enabled        = true;
        desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
        desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE;
        // desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
        // desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE;
    }

    if (pip_blend & BL_Alpha)
    {
        desc.colors[0].blend.enabled        = true;
        desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
        desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        // desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
        // desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE;
    }

    desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;

    desc.layout.buffers[0].stride = sizeof(frame_vert_t);

    desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    desc.index_type     = SG_INDEXTYPE_UINT32;

    // Layout must match the single texture variation
    // todo, shader manager
    desc.layout.attrs[ATTR_world_vs_color0].format = SG_VERTEXFORMAT_UBYTE4N;
    desc.layout.attrs[ATTR_world_vs_color0].offset = offsetof(frame_vert_t, rgba);

    desc.layout.attrs[ATTR_world_vs_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_world_vs_position].offset = offsetof(frame_vert_t, pos);

    desc.layout.attrs[ATTR_world_vs_texcoords].format = SG_VERTEXFORMAT_FLOAT4;
    desc.layout.attrs[ATTR_world_vs_texcoords].offset = offsetof(frame_vert_t, texc);
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

        if (unit->pass > 0)
        {
            // continue;
        }

        if (unit->shape != GL_POLYGON || unit->count < 3)
        {
            continue;
        }

        sg_pipeline pip = GFX_GetDrawUnitPipeline(unit);

        uint32_t images[2]   = {0};
        uint32_t samplers[2] = {0};

        frame_pipelines.insert(pip.id);

        for (int j = 0; j < 2; j++)
        {
            if (unit->tex[j])
            {
                auto search = gfx_image_lookup.find(unit->tex[j]);
                SYS_ASSERT(search != gfx_image_lookup.end());
                gfx_image_t &gimage = search->second;
                images[j]           = gimage.image_id;
                samplers[j]         = gimage.sampler_id;

                if (unit->blending & BL_ClampY)
                {
                    if (!gimage.sampler_clamp_y_id)
                    {
                        // need a new sampler
                        sg_sampler_desc desc      = sg_query_sampler_desc(sg_sampler{samplers[j]});
                        desc.wrap_v               = SG_WRAP_CLAMP_TO_EDGE;
                        gimage.sampler_clamp_y_id = sg_make_sampler(&desc).id;
                    }
                    samplers[j] = gimage.sampler_clamp_y_id;
                }
            }
        }

        if (!cmd || cmd->pipeline.id != pip.id || cmd->images[0] != images[0] || cmd->images[1] != images[1] ||
            cmd->samplers[0] != samplers[0] || cmd->samplers[1] != samplers[1])
        {
            cmd              = &draw_commands[cur_command++];
            cmd->index_first = cur_frame_index;
            cmd->index_count = 0;
            cmd->pipeline    = pip;
            cmd->images[0]   = images[0];
            cmd->images[1]   = images[1];
            cmd->samplers[0] = samplers[0];
            cmd->samplers[1] = samplers[1];
        }
        if (unit->shape == GL_POLYGON)
        {
            uint32_t start_index = cur_frame_vert;

            local_gl_vert_t *local = local_verts + unit->first;
            for (int j = 0; j < unit->count; j++)
            {
                frame_vert_t    *dest = frame_vertices + cur_frame_vert;
                local_gl_vert_t *src  = &local[j];
                dest->pos             = src->pos;
                dest->texc[0]         = src->texc[0];
                dest->texc[1]         = src->texc[1];
                dest->rgba[0]         = uint8_t(MIN(src->rgba[0], 1.0f) * 255.0f);
                dest->rgba[1]         = uint8_t(MIN(src->rgba[1], 1.0f) * 255.0f);
                dest->rgba[2]         = uint8_t(MIN(src->rgba[2], 1.0f) * 255.0f);
                dest->rgba[3]         = uint8_t(MIN(src->rgba[3], 1.0f) * 255.0f);

                cur_frame_vert++;

                if (cur_frame_vert >= MAX_FRAME_VERTS)
                {
                    I_Error("Ran out of frame verts %i", cur_frame_vert);
                }
            }

            for (int j = 0; j < unit->count - 1; j++)
            {
                uint32_t *dest = frame_indices + cur_frame_index;

                *dest++ = start_index;
                *dest++ = start_index + j + 1;
                *dest++ = start_index + ((j + 2) % unit->count);

                cmd->index_count += 3;
                cur_frame_index += 3;

                if (cur_frame_index >= MAX_FRAME_INDICES)
                {
                    I_Error("Ran out of frame indices %i", cur_frame_index);
                }
            }
        }
    }

    SYS_ASSERT(cur_command < MAX_DRAW_COMMANDS);

    cur_local_vert = cur_unit = 0;
}
