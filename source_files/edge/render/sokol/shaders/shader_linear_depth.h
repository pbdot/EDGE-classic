#pragma once
/*
    #version:1# (machine generated, don't edit!)

    Generated by sokol-shdc (https://github.com/floooh/sokol-tools)

    Cmdline:
        sokol-shdc -i linear_depth.glsl -o shader_linear_depth.h -l glsl410:hlsl4:glsl300es -b

    Overview:
    =========
    Shader program: 'linear_depth':
        Get shader desc: linear_depth_shader_desc(sg_query_backend());
        Vertex Shader: linear_depth_vs
        Fragment Shader: linear_depth_fs
        Attributes:
            ATTR_linear_depth_v_position => 0
            ATTR_linear_depth_v_uv => 1
    Bindings:
        Uniform block 'linear_depth_params':
            C struct: linear_depth_params_t
            Bind slot: UB_linear_depth_params => 0
        Image 'color_texture':
            Image type: SG_IMAGETYPE_2D
            Sample type: SG_IMAGESAMPLETYPE_FLOAT
            Multisampled: false
            Bind slot: IMG_color_texture => 1
        Image 'depth_texture':
            Image type: SG_IMAGETYPE_2D
            Sample type: SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT
            Multisampled: false
            Bind slot: IMG_depth_texture => 0
        Sampler 'color_sampler':
            Type: SG_SAMPLERTYPE_FILTERING
            Bind slot: SMP_color_sampler => 1
        Sampler 'depth_sampler':
            Type: SG_SAMPLERTYPE_NONFILTERING
            Bind slot: SMP_depth_sampler => 0
*/
#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before shader_linear_depth.h"
#endif
#if !defined(SOKOL_SHDC_ALIGN)
#if defined(_MSC_VER)
#define SOKOL_SHDC_ALIGN(a) __declspec(align(a))
#else
#define SOKOL_SHDC_ALIGN(a) __attribute__((aligned(a)))
#endif
#endif
#define ATTR_linear_depth_v_position (0)
#define ATTR_linear_depth_v_uv (1)
#define UB_linear_depth_params (0)
#define IMG_color_texture (1)
#define IMG_depth_texture (0)
#define SMP_color_sampler (1)
#define SMP_depth_sampler (0)
#pragma pack(push,1)
SOKOL_SHDC_ALIGN(16) typedef struct linear_depth_params_t {
    float inverse_depth_range_a;
    float inverse_depth_range_b;
    float linearize_depth_a;
    float linearize_depth_b;
} linear_depth_params_t;
#pragma pack(pop)
/*
    #version 410

    layout(location = 0) in vec4 v_position;
    layout(location = 0) out vec2 uv;
    layout(location = 1) in vec2 v_uv;

    void main()
    {
        gl_Position = v_position;
        uv = v_uv;
    }

*/
static const uint8_t linear_depth_vs_source_glsl410[188] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x34,0x31,0x30,0x0a,0x0a,0x6c,0x61,
    0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,
    0x30,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x34,0x20,0x76,0x5f,0x70,0x6f,0x73,
    0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,
    0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,0x6f,0x75,0x74,0x20,
    0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,
    0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x31,0x29,0x20,0x69,0x6e,
    0x20,0x76,0x65,0x63,0x32,0x20,0x76,0x5f,0x75,0x76,0x3b,0x0a,0x0a,0x76,0x6f,0x69,
    0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x67,
    0x6c,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x76,0x5f,0x70,
    0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x20,0x20,0x20,0x20,0x75,0x76,0x20,
    0x3d,0x20,0x76,0x5f,0x75,0x76,0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #version 410

    uniform vec4 linear_depth_params[1];
    uniform sampler2D color_texture_color_sampler;
    uniform sampler2D depth_texture_depth_sampler;

    layout(location = 0) in vec2 uv;
    layout(location = 0) out vec4 frag_color;

    float normalizeDepth(float depth)
    {
        return 1.0 / fma(clamp(fma(linear_depth_params[0].x, depth, linear_depth_params[0].y), 0.0, 1.0), linear_depth_params[0].z, linear_depth_params[0].w);
    }

    void main()
    {
        float _64;
        if (texture(color_texture_color_sampler, uv, 0.0).w != 0.0)
        {
            _64 = texture(depth_texture_depth_sampler, uv, 0.0).x;
        }
        else
        {
            _64 = 1.0;
        }
        float param = _64;
        frag_color = vec4(normalizeDepth(param), 0.0, 0.0, 1.0);
    }

*/
static const uint8_t linear_depth_fs_source_glsl410[712] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x34,0x31,0x30,0x0a,0x0a,0x75,0x6e,
    0x69,0x66,0x6f,0x72,0x6d,0x20,0x76,0x65,0x63,0x34,0x20,0x6c,0x69,0x6e,0x65,0x61,
    0x72,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,
    0x5d,0x3b,0x0a,0x75,0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x73,0x61,0x6d,0x70,0x6c,
    0x65,0x72,0x32,0x44,0x20,0x63,0x6f,0x6c,0x6f,0x72,0x5f,0x74,0x65,0x78,0x74,0x75,
    0x72,0x65,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x5f,0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,
    0x3b,0x0a,0x75,0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x73,0x61,0x6d,0x70,0x6c,0x65,
    0x72,0x32,0x44,0x20,0x64,0x65,0x70,0x74,0x68,0x5f,0x74,0x65,0x78,0x74,0x75,0x72,
    0x65,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,0x3b,
    0x0a,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,
    0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x75,
    0x76,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,
    0x6f,0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,0x6f,0x75,0x74,0x20,0x76,0x65,0x63,0x34,
    0x20,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x0a,0x66,0x6c,
    0x6f,0x61,0x74,0x20,0x6e,0x6f,0x72,0x6d,0x61,0x6c,0x69,0x7a,0x65,0x44,0x65,0x70,
    0x74,0x68,0x28,0x66,0x6c,0x6f,0x61,0x74,0x20,0x64,0x65,0x70,0x74,0x68,0x29,0x0a,
    0x7b,0x0a,0x20,0x20,0x20,0x20,0x72,0x65,0x74,0x75,0x72,0x6e,0x20,0x31,0x2e,0x30,
    0x20,0x2f,0x20,0x66,0x6d,0x61,0x28,0x63,0x6c,0x61,0x6d,0x70,0x28,0x66,0x6d,0x61,
    0x28,0x6c,0x69,0x6e,0x65,0x61,0x72,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x70,0x61,
    0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x78,0x2c,0x20,0x64,0x65,0x70,0x74,0x68,
    0x2c,0x20,0x6c,0x69,0x6e,0x65,0x61,0x72,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x70,
    0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x79,0x29,0x2c,0x20,0x30,0x2e,0x30,
    0x2c,0x20,0x31,0x2e,0x30,0x29,0x2c,0x20,0x6c,0x69,0x6e,0x65,0x61,0x72,0x5f,0x64,
    0x65,0x70,0x74,0x68,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x7a,
    0x2c,0x20,0x6c,0x69,0x6e,0x65,0x61,0x72,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x70,
    0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x77,0x29,0x3b,0x0a,0x7d,0x0a,0x0a,
    0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,
    0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x5f,0x36,0x34,0x3b,0x0a,0x20,0x20,0x20,
    0x20,0x69,0x66,0x20,0x28,0x74,0x65,0x78,0x74,0x75,0x72,0x65,0x28,0x63,0x6f,0x6c,
    0x6f,0x72,0x5f,0x74,0x65,0x78,0x74,0x75,0x72,0x65,0x5f,0x63,0x6f,0x6c,0x6f,0x72,
    0x5f,0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,0x2c,0x20,0x75,0x76,0x2c,0x20,0x30,0x2e,
    0x30,0x29,0x2e,0x77,0x20,0x21,0x3d,0x20,0x30,0x2e,0x30,0x29,0x0a,0x20,0x20,0x20,
    0x20,0x7b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5f,0x36,0x34,0x20,0x3d,
    0x20,0x74,0x65,0x78,0x74,0x75,0x72,0x65,0x28,0x64,0x65,0x70,0x74,0x68,0x5f,0x74,
    0x65,0x78,0x74,0x75,0x72,0x65,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x73,0x61,0x6d,
    0x70,0x6c,0x65,0x72,0x2c,0x20,0x75,0x76,0x2c,0x20,0x30,0x2e,0x30,0x29,0x2e,0x78,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x7d,0x0a,0x20,0x20,0x20,0x20,0x65,0x6c,0x73,0x65,
    0x0a,0x20,0x20,0x20,0x20,0x7b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5f,
    0x36,0x34,0x20,0x3d,0x20,0x31,0x2e,0x30,0x3b,0x0a,0x20,0x20,0x20,0x20,0x7d,0x0a,
    0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x70,0x61,0x72,0x61,0x6d,0x20,
    0x3d,0x20,0x5f,0x36,0x34,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x72,0x61,0x67,0x5f,
    0x63,0x6f,0x6c,0x6f,0x72,0x20,0x3d,0x20,0x76,0x65,0x63,0x34,0x28,0x6e,0x6f,0x72,
    0x6d,0x61,0x6c,0x69,0x7a,0x65,0x44,0x65,0x70,0x74,0x68,0x28,0x70,0x61,0x72,0x61,
    0x6d,0x29,0x2c,0x20,0x30,0x2e,0x30,0x2c,0x20,0x30,0x2e,0x30,0x2c,0x20,0x31,0x2e,
    0x30,0x29,0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #version 300 es

    layout(location = 0) in vec4 v_position;
    out vec2 uv;
    layout(location = 1) in vec2 v_uv;

    void main()
    {
        gl_Position = v_position;
        uv = v_uv;
    }

*/
static const uint8_t linear_depth_vs_source_glsl300es[170] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x30,0x30,0x20,0x65,0x73,0x0a,
    0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,
    0x20,0x3d,0x20,0x30,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x34,0x20,0x76,0x5f,
    0x70,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x6f,0x75,0x74,0x20,0x76,0x65,
    0x63,0x32,0x20,0x75,0x76,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,
    0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x31,0x29,0x20,0x69,0x6e,0x20,0x76,
    0x65,0x63,0x32,0x20,0x76,0x5f,0x75,0x76,0x3b,0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,
    0x6d,0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x67,0x6c,0x5f,
    0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x76,0x5f,0x70,0x6f,0x73,
    0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x20,0x20,0x20,0x20,0x75,0x76,0x20,0x3d,0x20,
    0x76,0x5f,0x75,0x76,0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #version 300 es
    precision mediump float;
    precision highp int;

    uniform highp vec4 linear_depth_params[1];
    uniform highp sampler2D color_texture_color_sampler;
    uniform highp sampler2D depth_texture_depth_sampler;

    in highp vec2 uv;
    layout(location = 0) out highp vec4 frag_color;

    highp float normalizeDepth(highp float depth)
    {
        return 1.0 / (clamp(linear_depth_params[0].x * depth + linear_depth_params[0].y, 0.0, 1.0) * linear_depth_params[0].z + linear_depth_params[0].w);
    }

    void main()
    {
        highp float _64;
        if (texture(color_texture_color_sampler, uv, 0.0).w != 0.0)
        {
            _64 = texture(depth_texture_depth_sampler, uv, 0.0).x;
        }
        else
        {
            _64 = 1.0;
        }
        highp float param = _64;
        frag_color = vec4(normalizeDepth(param), 0.0, 0.0, 1.0);
    }

*/
static const uint8_t linear_depth_fs_source_glsl300es[790] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x30,0x30,0x20,0x65,0x73,0x0a,
    0x70,0x72,0x65,0x63,0x69,0x73,0x69,0x6f,0x6e,0x20,0x6d,0x65,0x64,0x69,0x75,0x6d,
    0x70,0x20,0x66,0x6c,0x6f,0x61,0x74,0x3b,0x0a,0x70,0x72,0x65,0x63,0x69,0x73,0x69,
    0x6f,0x6e,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x69,0x6e,0x74,0x3b,0x0a,0x0a,0x75,
    0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x76,0x65,0x63,
    0x34,0x20,0x6c,0x69,0x6e,0x65,0x61,0x72,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x70,
    0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x5d,0x3b,0x0a,0x75,0x6e,0x69,0x66,0x6f,0x72,
    0x6d,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,0x32,
    0x44,0x20,0x63,0x6f,0x6c,0x6f,0x72,0x5f,0x74,0x65,0x78,0x74,0x75,0x72,0x65,0x5f,
    0x63,0x6f,0x6c,0x6f,0x72,0x5f,0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,0x3b,0x0a,0x75,
    0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x73,0x61,0x6d,
    0x70,0x6c,0x65,0x72,0x32,0x44,0x20,0x64,0x65,0x70,0x74,0x68,0x5f,0x74,0x65,0x78,
    0x74,0x75,0x72,0x65,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x73,0x61,0x6d,0x70,0x6c,
    0x65,0x72,0x3b,0x0a,0x0a,0x69,0x6e,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x76,0x65,
    0x63,0x32,0x20,0x75,0x76,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,
    0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,0x6f,0x75,0x74,0x20,
    0x68,0x69,0x67,0x68,0x70,0x20,0x76,0x65,0x63,0x34,0x20,0x66,0x72,0x61,0x67,0x5f,
    0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x0a,0x68,0x69,0x67,0x68,0x70,0x20,0x66,0x6c,
    0x6f,0x61,0x74,0x20,0x6e,0x6f,0x72,0x6d,0x61,0x6c,0x69,0x7a,0x65,0x44,0x65,0x70,
    0x74,0x68,0x28,0x68,0x69,0x67,0x68,0x70,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x64,
    0x65,0x70,0x74,0x68,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x72,0x65,0x74,0x75,
    0x72,0x6e,0x20,0x31,0x2e,0x30,0x20,0x2f,0x20,0x28,0x63,0x6c,0x61,0x6d,0x70,0x28,
    0x6c,0x69,0x6e,0x65,0x61,0x72,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x70,0x61,0x72,
    0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x78,0x20,0x2a,0x20,0x64,0x65,0x70,0x74,0x68,
    0x20,0x2b,0x20,0x6c,0x69,0x6e,0x65,0x61,0x72,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,
    0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x79,0x2c,0x20,0x30,0x2e,0x30,
    0x2c,0x20,0x31,0x2e,0x30,0x29,0x20,0x2a,0x20,0x6c,0x69,0x6e,0x65,0x61,0x72,0x5f,
    0x64,0x65,0x70,0x74,0x68,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,
    0x7a,0x20,0x2b,0x20,0x6c,0x69,0x6e,0x65,0x61,0x72,0x5f,0x64,0x65,0x70,0x74,0x68,
    0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x77,0x29,0x3b,0x0a,0x7d,
    0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,
    0x20,0x20,0x20,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,
    0x5f,0x36,0x34,0x3b,0x0a,0x20,0x20,0x20,0x20,0x69,0x66,0x20,0x28,0x74,0x65,0x78,
    0x74,0x75,0x72,0x65,0x28,0x63,0x6f,0x6c,0x6f,0x72,0x5f,0x74,0x65,0x78,0x74,0x75,
    0x72,0x65,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x5f,0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,
    0x2c,0x20,0x75,0x76,0x2c,0x20,0x30,0x2e,0x30,0x29,0x2e,0x77,0x20,0x21,0x3d,0x20,
    0x30,0x2e,0x30,0x29,0x0a,0x20,0x20,0x20,0x20,0x7b,0x0a,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x5f,0x36,0x34,0x20,0x3d,0x20,0x74,0x65,0x78,0x74,0x75,0x72,0x65,
    0x28,0x64,0x65,0x70,0x74,0x68,0x5f,0x74,0x65,0x78,0x74,0x75,0x72,0x65,0x5f,0x64,
    0x65,0x70,0x74,0x68,0x5f,0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,0x2c,0x20,0x75,0x76,
    0x2c,0x20,0x30,0x2e,0x30,0x29,0x2e,0x78,0x3b,0x0a,0x20,0x20,0x20,0x20,0x7d,0x0a,
    0x20,0x20,0x20,0x20,0x65,0x6c,0x73,0x65,0x0a,0x20,0x20,0x20,0x20,0x7b,0x0a,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5f,0x36,0x34,0x20,0x3d,0x20,0x31,0x2e,0x30,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x7d,0x0a,0x20,0x20,0x20,0x20,0x68,0x69,0x67,0x68,
    0x70,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x70,0x61,0x72,0x61,0x6d,0x20,0x3d,0x20,
    0x5f,0x36,0x34,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,
    0x6c,0x6f,0x72,0x20,0x3d,0x20,0x76,0x65,0x63,0x34,0x28,0x6e,0x6f,0x72,0x6d,0x61,
    0x6c,0x69,0x7a,0x65,0x44,0x65,0x70,0x74,0x68,0x28,0x70,0x61,0x72,0x61,0x6d,0x29,
    0x2c,0x20,0x30,0x2e,0x30,0x2c,0x20,0x30,0x2e,0x30,0x2c,0x20,0x31,0x2e,0x30,0x29,
    0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    static float4 gl_Position;
    static float4 v_position;
    static float2 uv;
    static float2 v_uv;

    struct SPIRV_Cross_Input
    {
        float4 v_position : TEXCOORD0;
        float2 v_uv : TEXCOORD1;
    };

    struct SPIRV_Cross_Output
    {
        float2 uv : TEXCOORD0;
        float4 gl_Position : SV_Position;
    };

    void vert_main()
    {
        gl_Position = v_position;
        uv = v_uv;
    }

    SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
    {
        v_position = stage_input.v_position;
        v_uv = stage_input.v_uv;
        vert_main();
        SPIRV_Cross_Output stage_output;
        stage_output.gl_Position = gl_Position;
        stage_output.uv = uv;
        return stage_output;
    }
*/
static const uint8_t linear_depth_vs_bytecode_hlsl4[528] = {
    0x44,0x58,0x42,0x43,0xf3,0x25,0xac,0x26,0x16,0x6c,0x31,0x27,0xf5,0xbf,0x35,0x11,
    0xa2,0xe3,0x81,0xb9,0x01,0x00,0x00,0x00,0x10,0x02,0x00,0x00,0x05,0x00,0x00,0x00,
    0x34,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0xcc,0x00,0x00,0x00,0x24,0x01,0x00,0x00,
    0x94,0x01,0x00,0x00,0x52,0x44,0x45,0x46,0x44,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1c,0x00,0x00,0x00,0x00,0x04,0xfe,0xff,
    0x10,0x81,0x00,0x00,0x1c,0x00,0x00,0x00,0x4d,0x69,0x63,0x72,0x6f,0x73,0x6f,0x66,
    0x74,0x20,0x28,0x52,0x29,0x20,0x48,0x4c,0x53,0x4c,0x20,0x53,0x68,0x61,0x64,0x65,
    0x72,0x20,0x43,0x6f,0x6d,0x70,0x69,0x6c,0x65,0x72,0x20,0x31,0x30,0x2e,0x31,0x00,
    0x49,0x53,0x47,0x4e,0x44,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x08,0x00,0x00,0x00,
    0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x0f,0x0f,0x00,0x00,0x38,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x03,0x03,0x00,0x00,
    0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,0x00,0xab,0xab,0xab,0x4f,0x53,0x47,0x4e,
    0x50,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x38,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x03,0x0c,0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x03,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x0f,0x00,0x00,0x00,0x54,0x45,0x58,0x43,
    0x4f,0x4f,0x52,0x44,0x00,0x53,0x56,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,
    0x00,0xab,0xab,0xab,0x53,0x48,0x44,0x52,0x68,0x00,0x00,0x00,0x40,0x00,0x01,0x00,
    0x1a,0x00,0x00,0x00,0x5f,0x00,0x00,0x03,0xf2,0x10,0x10,0x00,0x00,0x00,0x00,0x00,
    0x5f,0x00,0x00,0x03,0x32,0x10,0x10,0x00,0x01,0x00,0x00,0x00,0x65,0x00,0x00,0x03,
    0x32,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x67,0x00,0x00,0x04,0xf2,0x20,0x10,0x00,
    0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x36,0x00,0x00,0x05,0x32,0x20,0x10,0x00,
    0x00,0x00,0x00,0x00,0x46,0x10,0x10,0x00,0x01,0x00,0x00,0x00,0x36,0x00,0x00,0x05,
    0xf2,0x20,0x10,0x00,0x01,0x00,0x00,0x00,0x46,0x1e,0x10,0x00,0x00,0x00,0x00,0x00,
    0x3e,0x00,0x00,0x01,0x53,0x54,0x41,0x54,0x74,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

};
/*
    cbuffer linear_depth_params : register(b0)
    {
        float _15_inverse_depth_range_a : packoffset(c0);
        float _15_inverse_depth_range_b : packoffset(c0.y);
        float _15_linearize_depth_a : packoffset(c0.z);
        float _15_linearize_depth_b : packoffset(c0.w);
    };

    Texture2D<float4> color_texture : register(t0);
    SamplerState color_sampler : register(s0);
    Texture2D<float4> depth_texture : register(t1);
    SamplerState depth_sampler : register(s1);

    static float2 uv;
    static float4 frag_color;

    struct SPIRV_Cross_Input
    {
        float2 uv : TEXCOORD0;
    };

    struct SPIRV_Cross_Output
    {
        float4 frag_color : SV_Target0;
    };

    float normalizeDepth(float depth)
    {
        return 1.0f / mad(clamp(mad(_15_inverse_depth_range_a, depth, _15_inverse_depth_range_b), 0.0f, 1.0f), _15_linearize_depth_a, _15_linearize_depth_b);
    }

    void frag_main()
    {
        float _64;
        if (color_texture.SampleBias(color_sampler, uv, 0.0f).w != 0.0f)
        {
            _64 = depth_texture.SampleBias(depth_sampler, uv, 0.0f).x;
        }
        else
        {
            _64 = 1.0f;
        }
        float param = _64;
        frag_color = float4(normalizeDepth(param), 0.0f, 0.0f, 1.0f);
    }

    SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
    {
        uv = stage_input.uv;
        frag_main();
        SPIRV_Cross_Output stage_output;
        stage_output.frag_color = frag_color;
        return stage_output;
    }
*/
static const uint8_t linear_depth_fs_bytecode_hlsl4[1268] = {
    0x44,0x58,0x42,0x43,0xc8,0x06,0x51,0xd0,0x30,0x69,0x69,0x74,0xd0,0x8c,0xf4,0x4e,
    0xb1,0x4c,0x7e,0xd8,0x01,0x00,0x00,0x00,0xf4,0x04,0x00,0x00,0x05,0x00,0x00,0x00,
    0x34,0x00,0x00,0x00,0x58,0x02,0x00,0x00,0x8c,0x02,0x00,0x00,0xc0,0x02,0x00,0x00,
    0x78,0x04,0x00,0x00,0x52,0x44,0x45,0x46,0x1c,0x02,0x00,0x00,0x01,0x00,0x00,0x00,
    0x08,0x01,0x00,0x00,0x05,0x00,0x00,0x00,0x1c,0x00,0x00,0x00,0x00,0x04,0xff,0xff,
    0x10,0x81,0x00,0x00,0xf2,0x01,0x00,0x00,0xbc,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xca,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xd8,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x05,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x0d,0x00,0x00,0x00,0xe6,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x05,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x01,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x0d,0x00,0x00,0x00,0xf4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x63,0x6f,0x6c,0x6f,0x72,0x5f,0x73,0x61,
    0x6d,0x70,0x6c,0x65,0x72,0x00,0x64,0x65,0x70,0x74,0x68,0x5f,0x73,0x61,0x6d,0x70,
    0x6c,0x65,0x72,0x00,0x63,0x6f,0x6c,0x6f,0x72,0x5f,0x74,0x65,0x78,0x74,0x75,0x72,
    0x65,0x00,0x64,0x65,0x70,0x74,0x68,0x5f,0x74,0x65,0x78,0x74,0x75,0x72,0x65,0x00,
    0x6c,0x69,0x6e,0x65,0x61,0x72,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x70,0x61,0x72,
    0x61,0x6d,0x73,0x00,0xf4,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x20,0x01,0x00,0x00,
    0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x01,0x00,0x00,
    0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x9c,0x01,0x00,0x00,
    0x00,0x00,0x00,0x00,0xac,0x01,0x00,0x00,0x04,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
    0x02,0x00,0x00,0x00,0x9c,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0xc6,0x01,0x00,0x00,
    0x08,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x9c,0x01,0x00,0x00,
    0x00,0x00,0x00,0x00,0xdc,0x01,0x00,0x00,0x0c,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
    0x02,0x00,0x00,0x00,0x9c,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x5f,0x31,0x35,0x5f,
    0x69,0x6e,0x76,0x65,0x72,0x73,0x65,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x72,0x61,
    0x6e,0x67,0x65,0x5f,0x61,0x00,0xab,0xab,0x00,0x00,0x03,0x00,0x01,0x00,0x01,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x5f,0x31,0x35,0x5f,0x69,0x6e,0x76,0x65,
    0x72,0x73,0x65,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x72,0x61,0x6e,0x67,0x65,0x5f,
    0x62,0x00,0x5f,0x31,0x35,0x5f,0x6c,0x69,0x6e,0x65,0x61,0x72,0x69,0x7a,0x65,0x5f,
    0x64,0x65,0x70,0x74,0x68,0x5f,0x61,0x00,0x5f,0x31,0x35,0x5f,0x6c,0x69,0x6e,0x65,
    0x61,0x72,0x69,0x7a,0x65,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x62,0x00,0x4d,0x69,
    0x63,0x72,0x6f,0x73,0x6f,0x66,0x74,0x20,0x28,0x52,0x29,0x20,0x48,0x4c,0x53,0x4c,
    0x20,0x53,0x68,0x61,0x64,0x65,0x72,0x20,0x43,0x6f,0x6d,0x70,0x69,0x6c,0x65,0x72,
    0x20,0x31,0x30,0x2e,0x31,0x00,0xab,0xab,0x49,0x53,0x47,0x4e,0x2c,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x00,
    0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,0x00,0xab,0xab,0xab,0x4f,0x53,0x47,0x4e,
    0x2c,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0f,0x00,0x00,0x00,0x53,0x56,0x5f,0x54,0x61,0x72,0x67,0x65,0x74,0x00,0xab,0xab,
    0x53,0x48,0x44,0x52,0xb0,0x01,0x00,0x00,0x40,0x00,0x00,0x00,0x6c,0x00,0x00,0x00,
    0x59,0x00,0x00,0x04,0x46,0x8e,0x20,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x5a,0x00,0x00,0x03,0x00,0x60,0x10,0x00,0x00,0x00,0x00,0x00,0x5a,0x00,0x00,0x03,
    0x00,0x60,0x10,0x00,0x01,0x00,0x00,0x00,0x58,0x18,0x00,0x04,0x00,0x70,0x10,0x00,
    0x00,0x00,0x00,0x00,0x55,0x55,0x00,0x00,0x58,0x18,0x00,0x04,0x00,0x70,0x10,0x00,
    0x01,0x00,0x00,0x00,0x55,0x55,0x00,0x00,0x62,0x10,0x00,0x03,0x32,0x10,0x10,0x00,
    0x00,0x00,0x00,0x00,0x65,0x00,0x00,0x03,0xf2,0x20,0x10,0x00,0x00,0x00,0x00,0x00,
    0x68,0x00,0x00,0x02,0x01,0x00,0x00,0x00,0x4a,0x00,0x00,0x0b,0xf2,0x00,0x10,0x00,
    0x00,0x00,0x00,0x00,0x46,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x46,0x7e,0x10,0x00,
    0x00,0x00,0x00,0x00,0x00,0x60,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x40,0x00,0x00,
    0x00,0x00,0x00,0x00,0x39,0x00,0x00,0x07,0x12,0x00,0x10,0x00,0x00,0x00,0x00,0x00,
    0x3a,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
    0x1f,0x00,0x04,0x03,0x0a,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x4a,0x00,0x00,0x0b,
    0xf2,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x46,0x10,0x10,0x00,0x00,0x00,0x00,0x00,
    0x46,0x7e,0x10,0x00,0x01,0x00,0x00,0x00,0x00,0x60,0x10,0x00,0x01,0x00,0x00,0x00,
    0x01,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x12,0x00,0x00,0x01,0x36,0x00,0x00,0x05,
    0x12,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x40,0x00,0x00,0x00,0x00,0x80,0x3f,
    0x15,0x00,0x00,0x01,0x32,0x20,0x00,0x0b,0x12,0x00,0x10,0x00,0x00,0x00,0x00,0x00,
    0x0a,0x80,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0x10,0x00,
    0x00,0x00,0x00,0x00,0x1a,0x80,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x32,0x00,0x00,0x0b,0x12,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0x10,0x00,
    0x00,0x00,0x00,0x00,0x2a,0x80,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x3a,0x80,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0e,0x00,0x00,0x0a,
    0x12,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x02,0x40,0x00,0x00,0x00,0x00,0x80,0x3f,
    0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,0x0a,0x00,0x10,0x00,
    0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x08,0xe2,0x20,0x10,0x00,0x00,0x00,0x00,0x00,
    0x02,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x80,0x3f,0x3e,0x00,0x00,0x01,0x53,0x54,0x41,0x54,0x74,0x00,0x00,0x00,
    0x0c,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
};
static inline const sg_shader_desc* linear_depth_shader_desc(sg_backend backend) {
    if (backend == SG_BACKEND_GLCORE) {
        static sg_shader_desc desc;
        static bool valid;
        if (!valid) {
            valid = true;
            desc.vertex_func.source = (const char*)linear_depth_vs_source_glsl410;
            desc.vertex_func.entry = "main";
            desc.fragment_func.source = (const char*)linear_depth_fs_source_glsl410;
            desc.fragment_func.entry = "main";
            desc.attrs[0].glsl_name = "v_position";
            desc.attrs[1].glsl_name = "v_uv";
            desc.uniform_blocks[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
            desc.uniform_blocks[0].size = 16;
            desc.uniform_blocks[0].glsl_uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
            desc.uniform_blocks[0].glsl_uniforms[0].array_count = 1;
            desc.uniform_blocks[0].glsl_uniforms[0].glsl_name = "linear_depth_params";
            desc.images[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.images[0].image_type = SG_IMAGETYPE_2D;
            desc.images[0].sample_type = SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT;
            desc.images[0].multisampled = false;
            desc.images[1].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.images[1].image_type = SG_IMAGETYPE_2D;
            desc.images[1].sample_type = SG_IMAGESAMPLETYPE_FLOAT;
            desc.images[1].multisampled = false;
            desc.samplers[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.samplers[0].sampler_type = SG_SAMPLERTYPE_NONFILTERING;
            desc.samplers[1].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.samplers[1].sampler_type = SG_SAMPLERTYPE_FILTERING;
            desc.image_sampler_pairs[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.image_sampler_pairs[0].image_slot = 1;
            desc.image_sampler_pairs[0].sampler_slot = 1;
            desc.image_sampler_pairs[0].glsl_name = "color_texture_color_sampler";
            desc.image_sampler_pairs[1].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.image_sampler_pairs[1].image_slot = 0;
            desc.image_sampler_pairs[1].sampler_slot = 0;
            desc.image_sampler_pairs[1].glsl_name = "depth_texture_depth_sampler";
            desc.label = "linear_depth_shader";
        }
        return &desc;
    }
    if (backend == SG_BACKEND_GLES3) {
        static sg_shader_desc desc;
        static bool valid;
        if (!valid) {
            valid = true;
            desc.vertex_func.source = (const char*)linear_depth_vs_source_glsl300es;
            desc.vertex_func.entry = "main";
            desc.fragment_func.source = (const char*)linear_depth_fs_source_glsl300es;
            desc.fragment_func.entry = "main";
            desc.attrs[0].glsl_name = "v_position";
            desc.attrs[1].glsl_name = "v_uv";
            desc.uniform_blocks[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
            desc.uniform_blocks[0].size = 16;
            desc.uniform_blocks[0].glsl_uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
            desc.uniform_blocks[0].glsl_uniforms[0].array_count = 1;
            desc.uniform_blocks[0].glsl_uniforms[0].glsl_name = "linear_depth_params";
            desc.images[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.images[0].image_type = SG_IMAGETYPE_2D;
            desc.images[0].sample_type = SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT;
            desc.images[0].multisampled = false;
            desc.images[1].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.images[1].image_type = SG_IMAGETYPE_2D;
            desc.images[1].sample_type = SG_IMAGESAMPLETYPE_FLOAT;
            desc.images[1].multisampled = false;
            desc.samplers[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.samplers[0].sampler_type = SG_SAMPLERTYPE_NONFILTERING;
            desc.samplers[1].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.samplers[1].sampler_type = SG_SAMPLERTYPE_FILTERING;
            desc.image_sampler_pairs[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.image_sampler_pairs[0].image_slot = 1;
            desc.image_sampler_pairs[0].sampler_slot = 1;
            desc.image_sampler_pairs[0].glsl_name = "color_texture_color_sampler";
            desc.image_sampler_pairs[1].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.image_sampler_pairs[1].image_slot = 0;
            desc.image_sampler_pairs[1].sampler_slot = 0;
            desc.image_sampler_pairs[1].glsl_name = "depth_texture_depth_sampler";
            desc.label = "linear_depth_shader";
        }
        return &desc;
    }
    if (backend == SG_BACKEND_D3D11) {
        static sg_shader_desc desc;
        static bool valid;
        if (!valid) {
            valid = true;
            desc.vertex_func.bytecode.ptr = linear_depth_vs_bytecode_hlsl4;
            desc.vertex_func.bytecode.size = 528;
            desc.vertex_func.entry = "main";
            desc.fragment_func.bytecode.ptr = linear_depth_fs_bytecode_hlsl4;
            desc.fragment_func.bytecode.size = 1268;
            desc.fragment_func.entry = "main";
            desc.attrs[0].hlsl_sem_name = "TEXCOORD";
            desc.attrs[0].hlsl_sem_index = 0;
            desc.attrs[1].hlsl_sem_name = "TEXCOORD";
            desc.attrs[1].hlsl_sem_index = 1;
            desc.uniform_blocks[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
            desc.uniform_blocks[0].size = 16;
            desc.uniform_blocks[0].hlsl_register_b_n = 0;
            desc.images[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.images[0].image_type = SG_IMAGETYPE_2D;
            desc.images[0].sample_type = SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT;
            desc.images[0].multisampled = false;
            desc.images[0].hlsl_register_t_n = 1;
            desc.images[1].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.images[1].image_type = SG_IMAGETYPE_2D;
            desc.images[1].sample_type = SG_IMAGESAMPLETYPE_FLOAT;
            desc.images[1].multisampled = false;
            desc.images[1].hlsl_register_t_n = 0;
            desc.samplers[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.samplers[0].sampler_type = SG_SAMPLERTYPE_NONFILTERING;
            desc.samplers[0].hlsl_register_s_n = 1;
            desc.samplers[1].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.samplers[1].sampler_type = SG_SAMPLERTYPE_FILTERING;
            desc.samplers[1].hlsl_register_s_n = 0;
            desc.image_sampler_pairs[0].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.image_sampler_pairs[0].image_slot = 1;
            desc.image_sampler_pairs[0].sampler_slot = 1;
            desc.image_sampler_pairs[1].stage = SG_SHADERSTAGE_FRAGMENT;
            desc.image_sampler_pairs[1].image_slot = 0;
            desc.image_sampler_pairs[1].sampler_slot = 0;
            desc.label = "linear_depth_shader";
        }
        return &desc;
    }
    return 0;
}
