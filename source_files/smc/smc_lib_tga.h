//------------------------------------------------------------------------
//  TGA (Targa) IMAGE LOADING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2013-2016 Andrew Apted
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#pragma once

#include "smc_im_color.h"

namespace smc
{

rgba_color_t *TGA_DecodeImage(const byte *buffer, size_t length, int &width, int &height);

void TGA_FreeImage(rgba_color_t *pixels);

} // namespace smc