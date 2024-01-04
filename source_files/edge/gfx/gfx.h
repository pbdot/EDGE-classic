#pragma once

#include "i_defs_gl.h"

namespace epi
{
class image_data_c;
}

struct gfx_image_t
{
    uint32_t image_id;
    uint32_t sampler_id;
};

void GFX_Setup();
void GFX_Frame();

GLuint GFX_UploadTexture(epi::image_data_c *img, int flags, int max_pix);
extern std::unordered_map<uint32_t, gfx_image_t> gfx_image_lookup;
