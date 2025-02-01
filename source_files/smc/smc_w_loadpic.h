//------------------------------------------------------------------------
//  WAD PIC LOADER
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#pragma once

#include "smc_im_img.h"
#include "smc_w_wad.h"

namespace smc
{

bool LoadPicture(Img_c &dest, Lump_c *lump,     /* Lump containing picture */
                 const char *pic_name,          /* Picture name, for messages */
                 int         pic_x_offset,      /* Coordinates of top left corner of picture */
                 int         pic_y_offset,      /* relative to top left corner of buffer. */
                 int        *pic_width  = NULL, /* To return the size of the picture */
                 int        *pic_height = NULL);       /* (can be NULL) */

#ifdef _FLTK_DISABLED
Img_c *LoadImage_PNG(Lump_c *lump, const char *name);
Img_c *LoadImage_JPEG(Lump_c *lump, const char *name);
#endif
Img_c *LoadImage_TGA(Lump_c *lump, const char *name);

// Determine the image format of the given wad lump.
//
// Return values are:
//    'p'  : PNG format
//    't'  : TGA (Targa) format
//    'd'  : Doom patch
//
//    'j'  : JPEG
//    'g'  : GIF
//    's'  : DDS
//
//    NUL  : unrecognized
//
char W_DetectImageFormat(Lump_c *lump);

} // namespace smc