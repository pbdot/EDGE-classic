#pragma once

#include <stdint.h>
#include "HandmadeMath.h"

namespace gfx
{

typedef uint32_t fragment_id;

enum framement_type
{
    GFX_FRAGMENT_2D,
    GFX_FRAGMENT_STATIC,
    GFX_FRAGMENT_DYNAMIC
};


struct fragment_vertex_t
{
    HMM_Vec3 pos;
    HMM_Vec2 uv;
    HMM_Vec4 rgba;
};

struct fragment_t
{
    fragment_vertex_t *vertices;
    uint16_t          *num_vertices;

    uint16_t *indices;
    uint16_t *num_indices;
};

} // namespace gfx
