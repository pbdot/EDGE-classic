#include "sk_state.h"

SokolRenderState state;

RenderState *global_render_state = &state;


void SokolRenderState::Initialize()
{
        // Default sample
    sg_sampler_desc sdesc = {0};

    sdesc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    sdesc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;

    // filtering
    sdesc.mag_filter    = SG_FILTER_NEAREST;
    sdesc.min_filter    = SG_FILTER_NEAREST;
    sdesc.mipmap_filter = SG_FILTER_NEAREST;

    default_sampler = sg_make_sampler(&sdesc);

    sg_sampler sampler = {0};

}