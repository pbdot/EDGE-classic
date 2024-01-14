//----------------------------------------------------------------------------
//  EDGE System Specific Header for OpenGL
//----------------------------------------------------------------------------
//
//  Copyright (c) 2007-2024 The EDGE Team.
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

#ifndef __SYSTEM_SPECIFIC_DEFS_OPENGL__
#define __SYSTEM_SPECIFIC_DEFS_OPENGL__

#include "glad/glad.h" // GLAD or gl4es

#ifdef EDGE_GL_ES2
#include "gl4esinit.h"
#endif

#define USING_GL_TYPES 1

#endif /* __SYSTEM_SPECIFIC_DEFS_OPENGL__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
