

#include "i_defs.h"
#include "i_defs_gl.h"

#include <vector>
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

// TODO review if these should be archived
DEF_CVAR(r_colorlighting, "1", 0)
DEF_CVAR(r_colormaterial, "1", 0)

#ifdef APPLE_SILICON
#define DUMB_CLAMP "1"
#else
#define DUMB_CLAMP "0"
#endif

DEF_CVAR(r_dumbsky, "1", 0)
DEF_CVAR(r_dumbmulti, "0", 0)
DEF_CVAR(r_dumbcombine, "0", 0)
DEF_CVAR(r_dumbclamp, DUMB_CLAMP, 0)

#define MAX_L_VERT 65545
#define MAX_L_UNIT 1024

#define DUMMY_CLAMP 789

extern cvar_c r_culling;
extern cvar_c r_cullfog;

rgbcol_t current_fog_rgb = RGB_NO_VALUE;
GLfloat  current_fog_color[4];
float    current_fog_density = 0;
GLfloat  cull_fog_color[4];

struct RenderState
{
    GLuint env1;
    GLuint tex1;
    GLuint env2;
    GLuint tex2;

    int      blending;
    int      pass;
    rgbcol_t fog_color;
    float    fog_density;
};

inline bool operator==(const RenderState &lhs, const RenderState &rhs)
{
    return lhs.env1 == rhs.env1 && lhs.tex1 == rhs.tex1 && lhs.env2 == rhs.env2 && lhs.tex2 == rhs.tex2 &&
           lhs.blending == rhs.blending && lhs.pass == rhs.pass && lhs.fog_color == rhs.fog_color &&
           AlmostEquals(lhs.fog_density, rhs.fog_density);
}

#define MAX_COMMANDS 1024

struct Command
{
    int shape;
    int start;
    int count;
};

struct RenderCommand
{
    bool enabled;

    RenderState state;
    // need to work passes in
    // int pass;

    local_gl_vert_t *vertices;
    int              num_verts;
    int              cur_verts;
    int              cur_shape;

    Command commands[65536];
    int     num_commands;
};

static RenderCommand rcommands[MAX_COMMANDS];

void RGL_InitUnits(void)
{
    memset(rcommands, 0, sizeof(RenderCommand) * MAX_COMMANDS);
}

void RGL_SoftInitUnits(void)
{
}

void RGL_StartUnits(bool sort_em)
{
}

void RGL_FinishUnits(void)
{
    RGL_DrawUnits();
}

void RGL_FinishFrameUnits(void)
{
}

void RGL_DrawUnits(void)
{
    for (int i = 0; i < MAX_COMMANDS; i++)
    {
        RenderCommand *c = &rcommands[i];
        if (!c->enabled)
        {
            break;
        }

        const RenderState *s = &c->state;        

        bool render = true;

        if (render)
        {
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_ALPHA_TEST);
            glDisable(GL_BLEND);

            glAlphaFunc(GL_GREATER, 0);

            glPolygonOffset(0, 0);

            ecframe_stats.draw_runits++;

            if (s->tex1 != 0)
            {
                glEnable(GL_TEXTURE_2D);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, s->tex1);
            }

            for (int j = 0; j < c->num_commands; j++)
            {
                const Command *cmd = &c->commands[j];

                local_gl_vert_t *v = &c->vertices[cmd->start];                

                glBegin(cmd->shape);

                for (int k = 0; k < cmd->count; k++)
                {

                    glColor4fv(v->rgba);

                    glMultiTexCoord2fv(GL_TEXTURE0, (const GLfloat *)&v->texc[0]);

                    // glMultiTexCoord2fv(GL_TEXTURE1, reinterpret_cast<const GLfloat *>(&v->texc[1]));

                    glNormal3fv((const GLfloat *)(&v->normal));

                    // vertex must be last
                    glVertex3fv((const GLfloat *)(&v->pos));

                    v++;
                }

                glEnd();
            }
        }

        c->enabled      = false;
        c->cur_verts    = 0;
        c->num_commands = 0;
        memset(&c->state, 0, sizeof(RenderState));
    }
}

static RenderCommand *ccommand = nullptr;

local_gl_vert_t *RGL_BeginUnit(GLuint shape, int max_vert, GLuint env1, GLuint tex1, GLuint env2, GLuint tex2, int pass,
                               int blending, rgbcol_t fog_color, float fog_density)
{
    SYS_ASSERT(!ccommand);

    RenderState cstate;

    cstate.env1 = env1;
    cstate.tex1 = tex1;
    cstate.env2 = env2;
    cstate.tex2 = tex2;

    cstate.blending    = blending;
    cstate.pass        = pass;
    cstate.fog_color   = fog_color;
    cstate.fog_density = fog_density;

    for (int i = 0; i < MAX_COMMANDS; i++)
    {
        RenderCommand *c = &rcommands[i];
        if (!c->enabled)
        {
            ccommand = c;
            memcpy(&c->state, &cstate, sizeof(RenderState));
            break;
        }

        if (cstate == c->state)
        {
            ccommand = c;
            break;
        }
    }

    SYS_ASSERT(ccommand);
    SYS_ASSERT(!ccommand->cur_shape);
    ccommand->enabled   = true;
    ccommand->cur_shape = shape;

    if ((ccommand->cur_verts + max_vert) >= ccommand->num_verts)
    {
        ccommand->num_verts = (ccommand->cur_verts + max_vert + 1024);

        if (ccommand->vertices)
        {
            ccommand->vertices =
                (local_gl_vert_t *)realloc(ccommand->vertices, ccommand->num_verts * sizeof(local_gl_vert_t));
        }
        else
        {
            ccommand->vertices = (local_gl_vert_t *)malloc(ccommand->num_verts * sizeof(local_gl_vert_t));
        }
    }

    local_gl_vert_t *ret = &ccommand->vertices[ccommand->cur_verts];

    return ret;
}
void RGL_EndUnit(int actual_vert)
{
    SYS_ASSERT(ccommand);
    SYS_ASSERT(ccommand->cur_shape);

    SYS_ASSERT(ccommand->num_commands < 65536);

    Command *c = &ccommand->commands[ccommand->num_commands++];
    c->shape   = ccommand->cur_shape;
    c->start   = ccommand->cur_verts;
    c->count   = actual_vert;

    ccommand->cur_verts += actual_vert;
    ccommand->cur_shape = 0;

    ccommand = nullptr;
}
