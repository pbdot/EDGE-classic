//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Things)
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2023  The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "i_defs_gl.h"

#include <math.h>

#include "math_color.h"
#include "image_data.h"
#include "image_funcs.h"
#include "str_util.h"

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "g_game.h" //currmap
#include "p_local.h"
#include "r_colormap.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_effects.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_texgl.h"
#include "w_sprite.h"

#include "m_misc.h" // !!!! model test

#include "AlmostEquals.h"

#include "coal.h"
#include "vm_coal.h"
#include "script/compat/lua_compat.h"

#include "edge_profiling.h"

extern coal::vm_c *ui_vm;
extern double      VM_GetFloat(coal::vm_c *vm, const char *mod_name, const char *var_name);

extern bool erraticism_active;

#define DEBUG 0

DEF_CVAR(r_crosshair, "0", CVAR_ARCHIVE)     // shape
DEF_CVAR(r_crosscolor, "0", CVAR_ARCHIVE)    // 0 .. 7
DEF_CVAR(r_crosssize, "16.0", CVAR_ARCHIVE)  // pixels on a 320x200 screen
DEF_CVAR(r_crossbright, "1.0", CVAR_ARCHIVE) // 1.0 is normal

float sprite_skew;

// colour of the player's weapon
int rgl_weapon_r;
int rgl_weapon_g;
int rgl_weapon_b;

extern mobj_t *view_cam_mo;
extern float   view_expand_w;

// The minimum distance between player and a visible sprite.
#define MINZ (4.0f)

static const image_c *crosshair_image;
static int            crosshair_which;

void RGL_DrawWeaponSprites(player_t *p)
{
}

void RGL_DrawCrosshair(player_t *p)
{
}

// ============================================================================
// R2_BSP START
// ============================================================================

int sprite_kludge = 0;

const image_c *R2_GetOtherSprite(int spritenum, int framenum, bool *flip)
{
    /* Used for non-object stuff, like weapons and finale */

    if (spritenum == SPR_NULL)
        return NULL;

    spriteframe_c *frame = W_GetSpriteFrame(spritenum, framenum);

    if (!frame || !frame->images[0])
    {
        (*flip) = false;
        return W_ImageForDummySprite();
    }

    *flip = frame->flip[0] ? true : false;

    return frame->images[0];
}

#define SY_FUDGE 2

void RGL_WalkThing(drawsub_c *dsub, mobj_t *mo)
{
}

void RGL_DrawThing(drawfloor_t *dfloor, drawthing_t *dthing)
{
}
void RGL_DrawSortThings(drawfloor_t *dfloor)
{
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
