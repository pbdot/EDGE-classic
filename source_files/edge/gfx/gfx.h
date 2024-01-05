#pragma once

#include "i_defs_gl.h"
#include "m_math.h"

namespace epi
{
class image_data_c;
}

struct gfx_image_t
{
    uint32_t image_id;
    uint32_t sampler_id;
    uint32_t sampler_clamp_y_id;
};

// a single vertex to pass to the GL
struct frame_vert_t
{
    uint8_t rgba[4];
    vec3_t  pos;
    vec2_t  texc[2];
};


void GFX_Setup();
void GFX_StartFrame();

GLuint GFX_UploadTexture(epi::image_data_c *img, int flags, int max_pix);
extern std::unordered_map<uint32_t, gfx_image_t> gfx_image_lookup;
