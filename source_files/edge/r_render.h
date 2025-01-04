#pragma once

#include "r_gldefs.h"
#include "r_image.h"
#include "r_units.h"

void RenderSubList(std::list<DrawSubsector *> &dsubs, bool for_mirror = false);

void BspWalkNode(unsigned int);

inline BlendingMode GetBlending(float alpha, ImageOpacity opacity)
{
    int blending;

    if (alpha >= 0.99f && opacity == kOpacitySolid)
        blending = kBlendingNone;
    else if (alpha >= 0.99f && opacity == kOpacityMasked)
        blending = kBlendingMasked;
    else
        blending = kBlendingLess;

    if (alpha < 0.99f || opacity == kOpacityComplex)
        blending |= kBlendingAlpha;

    return (BlendingMode)blending;
}

#ifdef EDGE_SOKOL

constexpr int32_t kRenderItemBatchSize = 32;

enum kRenderType
{
    kRenderSubsector = 0,
    kRenderSkyWall,
    kRenderSkyPlane
};

struct RenderItem
{
    kRenderType type_;

    DrawSubsector *subsector_;

    Seg       *wallSeg_;
    Subsector *wallPlane_;

    float height1_;
    float height2_;
};

struct RenderBatch
{
    RenderItem items_[kRenderItemBatchSize];
    int32_t    num_items_;
};

void         BSPTraverse();
bool         BSPTraversing();
RenderBatch *BSPReadRenderBatch();

struct RenderContext
{
    Sector *front_sector;
    Sector *back_sector;

    int  swirl_pass   = 0;
    bool thick_liquid = false;

    Subsector     *current_subsector;
    DrawSubsector *current_draw_subsector;
    Seg           *current_seg;

    void Clear()
    {
        front_sector = nullptr;
        back_sector  = nullptr;

        swirl_pass   = 0;
        thick_liquid = false;

        current_subsector      = nullptr;
        current_draw_subsector = nullptr;
        current_seg            = nullptr;
    }

    UnitContext unit_context_;
};

#endif