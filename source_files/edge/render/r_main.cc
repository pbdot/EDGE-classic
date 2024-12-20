//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Main Stuff)
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2024 The EDGE Team.
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

EDGE_DEFINE_CONSOLE_VARIABLE(renderer_near_clip, "1", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(renderer_far_clip, "64000", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(draw_culling, "0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE_CLAMPED(draw_culling_distance, "3000", kConsoleVariableFlagArchive, 1000.0f, 16000.0f)
EDGE_DEFINE_CONSOLE_VARIABLE(cull_fog_color, "0", kConsoleVariableFlagArchive)
int maximum_texture_size;