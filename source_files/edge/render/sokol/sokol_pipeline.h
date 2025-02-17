#pragma once

#include "sokol_local.h"

enum PipelineFlags
{
    kPipelineDepthTest                  = 1 << 0,
    kPipelineDepthWrite                 = 1 << 1,
    kPipelineDepthGreater               = 1 << 2,
    kPipelineBlend                      = 1 << 3,
    kPipelineBlendSrc_SrcAlpha          = 1 << 4,
    kPipelineBlendSrc_OneMinusDestColor = 1 << 5,
    kPipelineBlendSrc_DstColor          = 1 << 6,
    kPipelineBlendSrc_Zero              = 1 << 7,
    kPipelineBlendDst_One               = 1 << 8,
    kPipelineBlendDst_OneMinusSrcAlpha  = 1 << 9,
    kPipelineBlendDst_SrcColor          = 1 << 10,
    kPipelineBlendDst_Zero              = 1 << 11,
    kPipelineCullFront                  = 1 << 12,
    kPipelineCullBack                   = 1 << 13,
    kPipelineSSAO                       = 1 << 14
};

void InitPipelines();

sgl_pipeline GetPipeline(sgl_context context, uint32_t pipeline_flags, GLenum src_blend = 0, GLenum dst_blend = 0);
