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

#pragma once

#pragma once
#ifdef EDGE_SOKOL
#include "render/sokol/sokol_gl_defines.h"
#elif EDGE_GL_ES2
#include "gl.h"
#include "gl4esinit.h"
#else
#include <glad/glad.h>
#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
