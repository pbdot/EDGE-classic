//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Skies)
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

#include "i_defs.h"
#include "i_defs_gl.h"

#include <math.h>

#include "image_data.h"

#include "dm_state.h"
#include "g_game.h" // currmap
#include "m_math.h"
#include "r_misc.h"
#include "w_flat.h"
#include "r_sky.h"
#include "r_gldefs.h"
#include "r_sky.h"
#include "r_colormap.h"
#include "r_modes.h"
#include "r_image.h"
#include "r_texgl.h"
#include "w_wad.h"

const image_c *sky_image;

bool custom_sky_box;

// needed for SKY
extern epi::image_data_c *ReadAsEpiBlock(image_c *rim);

extern cvar_c r_culling;

static GLfloat sky_cap_color[3];

static skystretch_e current_sky_stretch = SKS_Unset;

DEF_CVAR_CLAMPED(r_skystretch, "0", CVAR_ARCHIVE, 0, 3);

typedef struct sec_sky_ring_s
{
    // which group of connected skies (0 if none)
    int group;

    // link of sector in RING
    struct sec_sky_ring_s *next;
    struct sec_sky_ring_s *prev;

    // maximal sky height of group
    float max_h;
} sec_sky_ring_t;

//
// R_ComputeSkyHeights
//
// This routine computes the sky height field in sector_t, which is
// the maximal sky height over all sky sectors (ceiling only) which
// are joined by 2S linedefs.
//
// Algorithm: Initially all sky sectors are in individual groups.  Now
// we scan the linedef list.  For each 2-sectored line with sky on
// both sides, merge the two groups into one.  Simple :).  We can
// compute the maximal height of the group as we go.
//
void R_ComputeSkyHeights(void)
{
    int       i;
    line_t   *ld;
    sector_t *sec;

    // --- initialise ---

    sec_sky_ring_t *rings = new sec_sky_ring_t[numsectors];

    memset(rings, 0, numsectors * sizeof(sec_sky_ring_t));

    for (i = 0, sec = sectors; i < numsectors; i++, sec++)
    {
        if (!IS_SKY(sec->ceil))
            continue;

        rings[i].group = (i + 1);
        rings[i].next = rings[i].prev = rings + i;
        rings[i].max_h                = sec->c_h;

        // leave some room for tall sprites
        static const float SPR_H_MAX = 256.0f;

        if (sec->c_h < 30000.0f && (sec->c_h > sec->f_h) && (sec->c_h < sec->f_h + SPR_H_MAX))
        {
            rings[i].max_h = sec->f_h + SPR_H_MAX;
        }
    }

    // --- make the pass over linedefs ---

    for (i = 0, ld = lines; i < numlines; i++, ld++)
    {
        sector_t       *sec1, *sec2;
        sec_sky_ring_t *ring1, *ring2, *tmp_R;

        if (!ld->side[0] || !ld->side[1])
            continue;

        sec1 = ld->frontsector;
        sec2 = ld->backsector;

        SYS_ASSERT(sec1 && sec2);

        if (sec1 == sec2)
            continue;

        ring1 = rings + (sec1 - sectors);
        ring2 = rings + (sec2 - sectors);

        // we require sky on both sides
        if (ring1->group == 0 || ring2->group == 0)
            continue;

        // already in the same group ?
        if (ring1->group == ring2->group)
            continue;

        // swap sectors to ensure the lower group is added to the higher
        // group, since we don't need to update the `max_h' fields of the
        // highest group.

        if (ring1->max_h < ring2->max_h)
        {
            tmp_R = ring1;
            ring1 = ring2;
            ring2 = tmp_R;
        }

        // update the group numbers in the second group

        ring2->group = ring1->group;
        ring2->max_h = ring1->max_h;

        for (tmp_R = ring2->next; tmp_R != ring2; tmp_R = tmp_R->next)
        {
            tmp_R->group = ring1->group;
            tmp_R->max_h = ring1->max_h;
        }

        // merge 'em baby...

        ring1->next->prev = ring2;
        ring2->next->prev = ring1;

        tmp_R       = ring1->next;
        ring1->next = ring2->next;
        ring2->next = tmp_R;
    }

    // --- now store the results, and free up ---

    for (i = 0, sec = sectors; i < numsectors; i++, sec++)
    {
        if (rings[i].group > 0)
            sec->sky_h = rings[i].max_h;

#if 0 // DEBUG CODE
		L_WriteDebug("SKY: sec %d  group %d  max_h %1.1f\n", i,
				rings[i].group, rings[i].max_h);
#endif
    }

    delete[] rings;
}

//----------------------------------------------------------------------------

bool need_to_draw_sky = false;

typedef struct
{
    const image_c *base_sky;

    const colourmap_c *fx_colmap;

    int face_size;

    GLuint tex[6];

    // face images are only present for custom skyboxes.
    // pseudo skyboxes are generated outside of the image system.
    const image_c *face[6];
} fake_skybox_t;

static fake_skybox_t fake_box[2] = {{NULL, NULL, 1, {0, 0, 0, 0, 0, 0}, {NULL, NULL, NULL, NULL, NULL, NULL}},
                                    {NULL, NULL, 1, {0, 0, 0, 0, 0, 0}, {NULL, NULL, NULL, NULL, NULL, NULL}}};

static void DeleteSkyTexGroup(int SK)
{
    for (int i = 0; i < 6; i++)
    {
        if (fake_box[SK].tex[i] != 0)
        {
            glDeleteTextures(1, &fake_box[SK].tex[i]);
            fake_box[SK].tex[i] = 0;
        }
    }
}

void DeleteSkyTextures(void)
{
    for (int SK = 0; SK < 2; SK++)
    {
        fake_box[SK].base_sky  = NULL;
        fake_box[SK].fx_colmap = NULL;

        DeleteSkyTexGroup(SK);
    }
}

void RGL_BeginSky(void)
{
}

void RGL_FinishSky(void)
{

}

void RGL_DrawSkyPlane(subsector_t *sub, float h)
{
}

void RGL_DrawSkyWall(seg_t *seg, float h1, float h2)
{
}

//----------------------------------------------------------------------------
int RGL_UpdateSkyBoxTextures(void)
{
    return 0;
}

void RGL_PreCacheSky(void)
{

}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
