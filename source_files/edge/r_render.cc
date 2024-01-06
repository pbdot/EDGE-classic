//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (BSP Traversal)
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
#include <unordered_set>
#include <unordered_map>

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "g_game.h"
#include "m_bbox.h"
#include "p_local.h"
#include "r_defs.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_gldefs.h"
#include "r_colormap.h"
#include "r_effects.h"
#include "r_image.h"
#include "r_occlude.h"
#include "r_sky.h"
#include "r_things.h"

#include "n_network.h" // N_NetUpdate

#include "AlmostEquals.h"
#include "edge_profiling.h"

#define DEBUG 0

#define FLOOD_DIST   1024.0f
#define FLOOD_EXPAND 128.0f

#define DOOM_YSLOPE      (0.525)
#define DOOM_YSLOPE_FULL (0.625)

#define WAVETABLE_INCREMENT 0.0009765625

// #define DEBUG_GREET_NEIGHBOUR

DEF_CVAR(debug_hom, "0", CVAR_CHEAT)
DEF_CVAR(r_forceflatlighting, "0", CVAR_ARCHIVE)

extern cvar_c r_culling;
extern cvar_c r_doubleframes;

side_t   *sidedef;
line_t   *linedef;
sector_t *frontsector;
sector_t *backsector;

unsigned int root_node;

int detail_level = 1;
int use_dlights  = 0;

std::unordered_set<abstract_shader_c *> seen_dlights;

int  swirl_pass   = 0;
bool thick_liquid = false;

float view_x_slope;
float view_y_slope;

float wave_now;    // value for doing wave table lookups
float plane_z_bob; // for floor/ceiling bob DDFSECT stuff

// -ES- 1999/03/20 Different right & left side clip angles, for asymmetric FOVs.
angle_t clip_left, clip_right;
angle_t clip_scope;

mobj_t *view_cam_mo;

float view_expand_w;



void R_Render(int x, int y, int w, int h, mobj_t *camera, bool full_height, float expand_w)
{
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
