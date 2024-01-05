#pragma once
/*
    #version:1# (machine generated, don't edit!)

    Generated by sokol-shdc (https://github.com/floooh/sokol-tools)

    Cmdline: sokol-shdc -i pp_ssao.glsl -o pp_ssao.h -l glsl330

    Overview:

        Shader program 'ssao':
            Get shader desc: ssao_shader_desc(sg_query_backend());
            Vertex shader: ssao_vs
                Attribute slots:
                    ATTR_ssao_vs_v_position = 0
                    ATTR_ssao_vs_v_uv = 1
            Fragment shader: ssao_fs
                Uniform block 'ssao_params':
                    C struct: ssao_params_t
                    Bind slot: SLOT_ssao_params = 0
                Image 't_depth':
                    Type: SG_IMAGETYPE_2D
                    Sample Type: SG_IMAGESAMPLETYPE_FLOAT
                    Bind slot: SLOT_t_depth = 0
                Sampler 't_smp':
                    Type: SG_SAMPLERTYPE_FILTERING
                    Bind slot: SLOT_t_smp = 0
                Image Sampler Pair 't_depth_t_smp':
                    Image: t_depth
                    Sampler: t_smp


    Shader descriptor structs:

        sg_shader ssao = sg_make_shader(ssao_shader_desc(sg_query_backend()));

    Vertex attribute locations for vertex shader 'ssao_vs':

        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .layout = {
                .attrs = {
                    [ATTR_ssao_vs_v_position] = { ... },
                    [ATTR_ssao_vs_v_uv] = { ... },
                },
            },
            ...});


    Image bind slots, use as index in sg_bindings.vs.images[] or .fs.images[]

        SLOT_t_depth = 0;

    Sampler bind slots, use as index in sg_bindings.vs.sampler[] or .fs.samplers[]

        SLOT_t_smp = 0;

    Bind slot and C-struct for uniform block 'ssao_params':

        ssao_params_t ssao_params = {
            .u_near = ...;
            .u_far = ...;
            .u_target_size = ...;
            .u_mat_p = ...;
            .u_inv_mat_p = ...;
        };
        sg_apply_uniforms(SG_SHADERSTAGE_[VS|FS], SLOT_ssao_params, &SG_RANGE(ssao_params));

*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#if !defined(SOKOL_SHDC_ALIGN)
  #if defined(_MSC_VER)
    #define SOKOL_SHDC_ALIGN(a) __declspec(align(a))
  #else
    #define SOKOL_SHDC_ALIGN(a) __attribute__((aligned(a)))
  #endif
#endif
#define ATTR_ssao_vs_v_position (0)
#define ATTR_ssao_vs_v_uv (1)
#define SLOT_t_depth (0)
#define SLOT_t_smp (0)
#define SLOT_ssao_params (0)
#pragma pack(push,1)
SOKOL_SHDC_ALIGN(16) typedef struct ssao_params_t {
    float u_near;
    float u_far;
    float u_target_size[2];
    float u_mat_p[16];
    float u_inv_mat_p[16];
} ssao_params_t;
#pragma pack(pop)
/*
    #version 330
    
    layout(location = 0) in vec4 v_position;
    out vec2 uv;
    layout(location = 1) in vec2 v_uv;
    
    void main()
    {
        gl_Position = v_position;
        uv = v_uv;
    }
    
*/
static const char ssao_vs_source_glsl330[167] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x33,0x30,0x0a,0x0a,0x6c,0x61,
    0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,
    0x30,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x34,0x20,0x76,0x5f,0x70,0x6f,0x73,
    0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x6f,0x75,0x74,0x20,0x76,0x65,0x63,0x32,0x20,
    0x75,0x76,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,
    0x69,0x6f,0x6e,0x20,0x3d,0x20,0x31,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,
    0x20,0x76,0x5f,0x75,0x76,0x3b,0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,
    0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x67,0x6c,0x5f,0x50,0x6f,0x73,
    0x69,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x76,0x5f,0x70,0x6f,0x73,0x69,0x74,0x69,
    0x6f,0x6e,0x3b,0x0a,0x20,0x20,0x20,0x20,0x75,0x76,0x20,0x3d,0x20,0x76,0x5f,0x75,
    0x76,0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #version 330
    
    uniform vec4 ssao_params[9];
    uniform sampler2D t_depth_t_smp;
    
    in vec2 uv;
    layout(location = 0) out vec4 frag_color;
    float scaleDividedByCameraFar;
    
    float getDepth(vec2 t_uv)
    {
        return texture(t_depth_t_smp, t_uv).x;
    }
    
    float getViewZ(float depth)
    {
        return (ssao_params[0].x * ssao_params[0].y) / (depth - ssao_params[0].y);
    }
    
    vec3 getViewPosition(vec2 screenPosition, float depth, float viewZ)
    {
        return (mat4(ssao_params[5], ssao_params[6], ssao_params[7], ssao_params[8]) * (vec4((vec3(screenPosition, depth) - vec3(0.5)) * 2.0, 1.0) * (ssao_params[3].w * viewZ + ssao_params[4].w))).xyz;
    }
    
    vec3 getViewNormal(vec3 viewPosition, vec2 t_uv)
    {
        return normalize(cross(dFdx(viewPosition), dFdy(viewPosition)));
    }
    
    float rand(vec2 uv_1)
    {
        return fract(sin(mod(dot(uv_1, vec2(12.98980045318603515625, 78.233001708984375)), 3.1415927410125732421875)) * 43758.546875);
    }
    
    float pow2(float v)
    {
        return v * v;
    }
    
    float getOcclusion(vec3 centerViewPosition, vec3 centerViewNormal, vec3 sampleViewPosition)
    {
        vec3 _169 = sampleViewPosition - centerViewPosition;
        float _178 = scaleDividedByCameraFar * length(_169);
        float _194 = max(0.0, (dot(centerViewNormal, _169) / _178) - 0.20000000298023223876953125) / (1.0 + pow2(_178));
        float result = _194;
        if (_194 > 220.0)
        {
            result = 0.0;
        }
        return max(0.0, clamp(result, 1.10000002384185791015625, 20.0) * 0.076923079788684844970703125 + (-0.20000000298023223876953125));
    }
    
    float getAmbientOcclusion(vec3 centerViewPosition, float centerDepth)
    {
        scaleDividedByCameraFar = 1.0 / ssao_params[0].y;
        vec3 _218 = getViewNormal(centerViewPosition, uv);
        float angle = rand(uv) * 6.283185482025146484375;
        vec2 _243 = max(vec2(3.0) / ((ssao_params[0].zw * centerDepth) * 0.0500000007450580596923828125), vec2(5.0) / ssao_params[0].zw);
        vec2 radius = _243;
        float occlusionSum = 0.0;
        for (int i = 0; i < 5; i++)
        {
            float _259 = angle;
            vec2 _264 = radius;
            vec2 _272 = clamp(vec2(cos(_259), sin(_259)) * _264 + uv, vec2(9.9999999747524270787835121154785e-07), vec2(0.999998986721038818359375));
            radius = _264 + _243;
            angle = _259 + 3.769911289215087890625;
            float _281 = getDepth(_272);
            occlusionSum += getOcclusion(centerViewPosition, _218, getViewPosition(_272, _281 / ssao_params[0].y, getViewZ(_281)));
        }
        return occlusionSum * 0.20000000298023223876953125;
    }
    
    vec4 float_to_rgba(float v)
    {
        vec4 _60 = fract(vec4(1.0, 255.0, 65025.0, 160581376.0) * v);
        return (-_60.yzww) * vec4(0.0039215688593685626983642578125, 0.0039215688593685626983642578125, 0.0039215688593685626983642578125, 0.0) + _60;
    }
    
    void main()
    {
        float _308 = getDepth(uv);
        float _313 = _308 / ssao_params[0].y;
        float param = _308;
        float _326 = getAmbientOcclusion(getViewPosition(uv, _313, getViewZ(_308)), param);
        frag_color = (float_to_rgba(_326) * max(0.0, 0.100000001490116119384765625 - _313)) * 10.0;
    }
    
*/
static const char ssao_fs_source_glsl330[3004] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x33,0x30,0x0a,0x0a,0x75,0x6e,
    0x69,0x66,0x6f,0x72,0x6d,0x20,0x76,0x65,0x63,0x34,0x20,0x73,0x73,0x61,0x6f,0x5f,
    0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x39,0x5d,0x3b,0x0a,0x75,0x6e,0x69,0x66,0x6f,
    0x72,0x6d,0x20,0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,0x32,0x44,0x20,0x74,0x5f,0x64,
    0x65,0x70,0x74,0x68,0x5f,0x74,0x5f,0x73,0x6d,0x70,0x3b,0x0a,0x0a,0x69,0x6e,0x20,
    0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,
    0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,0x6f,0x75,
    0x74,0x20,0x76,0x65,0x63,0x34,0x20,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,
    0x72,0x3b,0x0a,0x66,0x6c,0x6f,0x61,0x74,0x20,0x73,0x63,0x61,0x6c,0x65,0x44,0x69,
    0x76,0x69,0x64,0x65,0x64,0x42,0x79,0x43,0x61,0x6d,0x65,0x72,0x61,0x46,0x61,0x72,
    0x3b,0x0a,0x0a,0x66,0x6c,0x6f,0x61,0x74,0x20,0x67,0x65,0x74,0x44,0x65,0x70,0x74,
    0x68,0x28,0x76,0x65,0x63,0x32,0x20,0x74,0x5f,0x75,0x76,0x29,0x0a,0x7b,0x0a,0x20,
    0x20,0x20,0x20,0x72,0x65,0x74,0x75,0x72,0x6e,0x20,0x74,0x65,0x78,0x74,0x75,0x72,
    0x65,0x28,0x74,0x5f,0x64,0x65,0x70,0x74,0x68,0x5f,0x74,0x5f,0x73,0x6d,0x70,0x2c,
    0x20,0x74,0x5f,0x75,0x76,0x29,0x2e,0x78,0x3b,0x0a,0x7d,0x0a,0x0a,0x66,0x6c,0x6f,
    0x61,0x74,0x20,0x67,0x65,0x74,0x56,0x69,0x65,0x77,0x5a,0x28,0x66,0x6c,0x6f,0x61,
    0x74,0x20,0x64,0x65,0x70,0x74,0x68,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x72,
    0x65,0x74,0x75,0x72,0x6e,0x20,0x28,0x73,0x73,0x61,0x6f,0x5f,0x70,0x61,0x72,0x61,
    0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x78,0x20,0x2a,0x20,0x73,0x73,0x61,0x6f,0x5f,0x70,
    0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x79,0x29,0x20,0x2f,0x20,0x28,0x64,
    0x65,0x70,0x74,0x68,0x20,0x2d,0x20,0x73,0x73,0x61,0x6f,0x5f,0x70,0x61,0x72,0x61,
    0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x79,0x29,0x3b,0x0a,0x7d,0x0a,0x0a,0x76,0x65,0x63,
    0x33,0x20,0x67,0x65,0x74,0x56,0x69,0x65,0x77,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,
    0x6e,0x28,0x76,0x65,0x63,0x32,0x20,0x73,0x63,0x72,0x65,0x65,0x6e,0x50,0x6f,0x73,
    0x69,0x74,0x69,0x6f,0x6e,0x2c,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x64,0x65,0x70,
    0x74,0x68,0x2c,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x76,0x69,0x65,0x77,0x5a,0x29,
    0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x72,0x65,0x74,0x75,0x72,0x6e,0x20,0x28,0x6d,
    0x61,0x74,0x34,0x28,0x73,0x73,0x61,0x6f,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,
    0x35,0x5d,0x2c,0x20,0x73,0x73,0x61,0x6f,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,
    0x36,0x5d,0x2c,0x20,0x73,0x73,0x61,0x6f,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,
    0x37,0x5d,0x2c,0x20,0x73,0x73,0x61,0x6f,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,
    0x38,0x5d,0x29,0x20,0x2a,0x20,0x28,0x76,0x65,0x63,0x34,0x28,0x28,0x76,0x65,0x63,
    0x33,0x28,0x73,0x63,0x72,0x65,0x65,0x6e,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,
    0x2c,0x20,0x64,0x65,0x70,0x74,0x68,0x29,0x20,0x2d,0x20,0x76,0x65,0x63,0x33,0x28,
    0x30,0x2e,0x35,0x29,0x29,0x20,0x2a,0x20,0x32,0x2e,0x30,0x2c,0x20,0x31,0x2e,0x30,
    0x29,0x20,0x2a,0x20,0x28,0x73,0x73,0x61,0x6f,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,
    0x5b,0x33,0x5d,0x2e,0x77,0x20,0x2a,0x20,0x76,0x69,0x65,0x77,0x5a,0x20,0x2b,0x20,
    0x73,0x73,0x61,0x6f,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x34,0x5d,0x2e,0x77,
    0x29,0x29,0x29,0x2e,0x78,0x79,0x7a,0x3b,0x0a,0x7d,0x0a,0x0a,0x76,0x65,0x63,0x33,
    0x20,0x67,0x65,0x74,0x56,0x69,0x65,0x77,0x4e,0x6f,0x72,0x6d,0x61,0x6c,0x28,0x76,
    0x65,0x63,0x33,0x20,0x76,0x69,0x65,0x77,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,
    0x2c,0x20,0x76,0x65,0x63,0x32,0x20,0x74,0x5f,0x75,0x76,0x29,0x0a,0x7b,0x0a,0x20,
    0x20,0x20,0x20,0x72,0x65,0x74,0x75,0x72,0x6e,0x20,0x6e,0x6f,0x72,0x6d,0x61,0x6c,
    0x69,0x7a,0x65,0x28,0x63,0x72,0x6f,0x73,0x73,0x28,0x64,0x46,0x64,0x78,0x28,0x76,
    0x69,0x65,0x77,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x29,0x2c,0x20,0x64,0x46,
    0x64,0x79,0x28,0x76,0x69,0x65,0x77,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x29,
    0x29,0x29,0x3b,0x0a,0x7d,0x0a,0x0a,0x66,0x6c,0x6f,0x61,0x74,0x20,0x72,0x61,0x6e,
    0x64,0x28,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x5f,0x31,0x29,0x0a,0x7b,0x0a,0x20,
    0x20,0x20,0x20,0x72,0x65,0x74,0x75,0x72,0x6e,0x20,0x66,0x72,0x61,0x63,0x74,0x28,
    0x73,0x69,0x6e,0x28,0x6d,0x6f,0x64,0x28,0x64,0x6f,0x74,0x28,0x75,0x76,0x5f,0x31,
    0x2c,0x20,0x76,0x65,0x63,0x32,0x28,0x31,0x32,0x2e,0x39,0x38,0x39,0x38,0x30,0x30,
    0x34,0x35,0x33,0x31,0x38,0x36,0x30,0x33,0x35,0x31,0x35,0x36,0x32,0x35,0x2c,0x20,
    0x37,0x38,0x2e,0x32,0x33,0x33,0x30,0x30,0x31,0x37,0x30,0x38,0x39,0x38,0x34,0x33,
    0x37,0x35,0x29,0x29,0x2c,0x20,0x33,0x2e,0x31,0x34,0x31,0x35,0x39,0x32,0x37,0x34,
    0x31,0x30,0x31,0x32,0x35,0x37,0x33,0x32,0x34,0x32,0x31,0x38,0x37,0x35,0x29,0x29,
    0x20,0x2a,0x20,0x34,0x33,0x37,0x35,0x38,0x2e,0x35,0x34,0x36,0x38,0x37,0x35,0x29,
    0x3b,0x0a,0x7d,0x0a,0x0a,0x66,0x6c,0x6f,0x61,0x74,0x20,0x70,0x6f,0x77,0x32,0x28,
    0x66,0x6c,0x6f,0x61,0x74,0x20,0x76,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x72,
    0x65,0x74,0x75,0x72,0x6e,0x20,0x76,0x20,0x2a,0x20,0x76,0x3b,0x0a,0x7d,0x0a,0x0a,
    0x66,0x6c,0x6f,0x61,0x74,0x20,0x67,0x65,0x74,0x4f,0x63,0x63,0x6c,0x75,0x73,0x69,
    0x6f,0x6e,0x28,0x76,0x65,0x63,0x33,0x20,0x63,0x65,0x6e,0x74,0x65,0x72,0x56,0x69,
    0x65,0x77,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x2c,0x20,0x76,0x65,0x63,0x33,
    0x20,0x63,0x65,0x6e,0x74,0x65,0x72,0x56,0x69,0x65,0x77,0x4e,0x6f,0x72,0x6d,0x61,
    0x6c,0x2c,0x20,0x76,0x65,0x63,0x33,0x20,0x73,0x61,0x6d,0x70,0x6c,0x65,0x56,0x69,
    0x65,0x77,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x29,0x0a,0x7b,0x0a,0x20,0x20,
    0x20,0x20,0x76,0x65,0x63,0x33,0x20,0x5f,0x31,0x36,0x39,0x20,0x3d,0x20,0x73,0x61,
    0x6d,0x70,0x6c,0x65,0x56,0x69,0x65,0x77,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,
    0x20,0x2d,0x20,0x63,0x65,0x6e,0x74,0x65,0x72,0x56,0x69,0x65,0x77,0x50,0x6f,0x73,
    0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,
    0x20,0x5f,0x31,0x37,0x38,0x20,0x3d,0x20,0x73,0x63,0x61,0x6c,0x65,0x44,0x69,0x76,
    0x69,0x64,0x65,0x64,0x42,0x79,0x43,0x61,0x6d,0x65,0x72,0x61,0x46,0x61,0x72,0x20,
    0x2a,0x20,0x6c,0x65,0x6e,0x67,0x74,0x68,0x28,0x5f,0x31,0x36,0x39,0x29,0x3b,0x0a,
    0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x5f,0x31,0x39,0x34,0x20,0x3d,
    0x20,0x6d,0x61,0x78,0x28,0x30,0x2e,0x30,0x2c,0x20,0x28,0x64,0x6f,0x74,0x28,0x63,
    0x65,0x6e,0x74,0x65,0x72,0x56,0x69,0x65,0x77,0x4e,0x6f,0x72,0x6d,0x61,0x6c,0x2c,
    0x20,0x5f,0x31,0x36,0x39,0x29,0x20,0x2f,0x20,0x5f,0x31,0x37,0x38,0x29,0x20,0x2d,
    0x20,0x30,0x2e,0x32,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x32,0x39,0x38,0x30,0x32,
    0x33,0x32,0x32,0x33,0x38,0x37,0x36,0x39,0x35,0x33,0x31,0x32,0x35,0x29,0x20,0x2f,
    0x20,0x28,0x31,0x2e,0x30,0x20,0x2b,0x20,0x70,0x6f,0x77,0x32,0x28,0x5f,0x31,0x37,
    0x38,0x29,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x72,
    0x65,0x73,0x75,0x6c,0x74,0x20,0x3d,0x20,0x5f,0x31,0x39,0x34,0x3b,0x0a,0x20,0x20,
    0x20,0x20,0x69,0x66,0x20,0x28,0x5f,0x31,0x39,0x34,0x20,0x3e,0x20,0x32,0x32,0x30,
    0x2e,0x30,0x29,0x0a,0x20,0x20,0x20,0x20,0x7b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x72,0x65,0x73,0x75,0x6c,0x74,0x20,0x3d,0x20,0x30,0x2e,0x30,0x3b,0x0a,
    0x20,0x20,0x20,0x20,0x7d,0x0a,0x20,0x20,0x20,0x20,0x72,0x65,0x74,0x75,0x72,0x6e,
    0x20,0x6d,0x61,0x78,0x28,0x30,0x2e,0x30,0x2c,0x20,0x63,0x6c,0x61,0x6d,0x70,0x28,
    0x72,0x65,0x73,0x75,0x6c,0x74,0x2c,0x20,0x31,0x2e,0x31,0x30,0x30,0x30,0x30,0x30,
    0x30,0x32,0x33,0x38,0x34,0x31,0x38,0x35,0x37,0x39,0x31,0x30,0x31,0x35,0x36,0x32,
    0x35,0x2c,0x20,0x32,0x30,0x2e,0x30,0x29,0x20,0x2a,0x20,0x30,0x2e,0x30,0x37,0x36,
    0x39,0x32,0x33,0x30,0x37,0x39,0x37,0x38,0x38,0x36,0x38,0x34,0x38,0x34,0x34,0x39,
    0x37,0x30,0x37,0x30,0x33,0x31,0x32,0x35,0x20,0x2b,0x20,0x28,0x2d,0x30,0x2e,0x32,
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x32,0x39,0x38,0x30,0x32,0x33,0x32,0x32,0x33,
    0x38,0x37,0x36,0x39,0x35,0x33,0x31,0x32,0x35,0x29,0x29,0x3b,0x0a,0x7d,0x0a,0x0a,
    0x66,0x6c,0x6f,0x61,0x74,0x20,0x67,0x65,0x74,0x41,0x6d,0x62,0x69,0x65,0x6e,0x74,
    0x4f,0x63,0x63,0x6c,0x75,0x73,0x69,0x6f,0x6e,0x28,0x76,0x65,0x63,0x33,0x20,0x63,
    0x65,0x6e,0x74,0x65,0x72,0x56,0x69,0x65,0x77,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,
    0x6e,0x2c,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x63,0x65,0x6e,0x74,0x65,0x72,0x44,
    0x65,0x70,0x74,0x68,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x73,0x63,0x61,0x6c,
    0x65,0x44,0x69,0x76,0x69,0x64,0x65,0x64,0x42,0x79,0x43,0x61,0x6d,0x65,0x72,0x61,
    0x46,0x61,0x72,0x20,0x3d,0x20,0x31,0x2e,0x30,0x20,0x2f,0x20,0x73,0x73,0x61,0x6f,
    0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x79,0x3b,0x0a,0x20,0x20,
    0x20,0x20,0x76,0x65,0x63,0x33,0x20,0x5f,0x32,0x31,0x38,0x20,0x3d,0x20,0x67,0x65,
    0x74,0x56,0x69,0x65,0x77,0x4e,0x6f,0x72,0x6d,0x61,0x6c,0x28,0x63,0x65,0x6e,0x74,
    0x65,0x72,0x56,0x69,0x65,0x77,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x2c,0x20,
    0x75,0x76,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x61,
    0x6e,0x67,0x6c,0x65,0x20,0x3d,0x20,0x72,0x61,0x6e,0x64,0x28,0x75,0x76,0x29,0x20,
    0x2a,0x20,0x36,0x2e,0x32,0x38,0x33,0x31,0x38,0x35,0x34,0x38,0x32,0x30,0x32,0x35,
    0x31,0x34,0x36,0x34,0x38,0x34,0x33,0x37,0x35,0x3b,0x0a,0x20,0x20,0x20,0x20,0x76,
    0x65,0x63,0x32,0x20,0x5f,0x32,0x34,0x33,0x20,0x3d,0x20,0x6d,0x61,0x78,0x28,0x76,
    0x65,0x63,0x32,0x28,0x33,0x2e,0x30,0x29,0x20,0x2f,0x20,0x28,0x28,0x73,0x73,0x61,
    0x6f,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x7a,0x77,0x20,0x2a,
    0x20,0x63,0x65,0x6e,0x74,0x65,0x72,0x44,0x65,0x70,0x74,0x68,0x29,0x20,0x2a,0x20,
    0x30,0x2e,0x30,0x35,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x37,0x34,0x35,0x30,0x35,
    0x38,0x30,0x35,0x39,0x36,0x39,0x32,0x33,0x38,0x32,0x38,0x31,0x32,0x35,0x29,0x2c,
    0x20,0x76,0x65,0x63,0x32,0x28,0x35,0x2e,0x30,0x29,0x20,0x2f,0x20,0x73,0x73,0x61,
    0x6f,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2e,0x7a,0x77,0x29,0x3b,
    0x0a,0x20,0x20,0x20,0x20,0x76,0x65,0x63,0x32,0x20,0x72,0x61,0x64,0x69,0x75,0x73,
    0x20,0x3d,0x20,0x5f,0x32,0x34,0x33,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,
    0x61,0x74,0x20,0x6f,0x63,0x63,0x6c,0x75,0x73,0x69,0x6f,0x6e,0x53,0x75,0x6d,0x20,
    0x3d,0x20,0x30,0x2e,0x30,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6f,0x72,0x20,0x28,
    0x69,0x6e,0x74,0x20,0x69,0x20,0x3d,0x20,0x30,0x3b,0x20,0x69,0x20,0x3c,0x20,0x35,
    0x3b,0x20,0x69,0x2b,0x2b,0x29,0x0a,0x20,0x20,0x20,0x20,0x7b,0x0a,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x5f,0x32,0x35,0x39,0x20,
    0x3d,0x20,0x61,0x6e,0x67,0x6c,0x65,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x76,0x65,0x63,0x32,0x20,0x5f,0x32,0x36,0x34,0x20,0x3d,0x20,0x72,0x61,0x64,
    0x69,0x75,0x73,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x76,0x65,0x63,
    0x32,0x20,0x5f,0x32,0x37,0x32,0x20,0x3d,0x20,0x63,0x6c,0x61,0x6d,0x70,0x28,0x76,
    0x65,0x63,0x32,0x28,0x63,0x6f,0x73,0x28,0x5f,0x32,0x35,0x39,0x29,0x2c,0x20,0x73,
    0x69,0x6e,0x28,0x5f,0x32,0x35,0x39,0x29,0x29,0x20,0x2a,0x20,0x5f,0x32,0x36,0x34,
    0x20,0x2b,0x20,0x75,0x76,0x2c,0x20,0x76,0x65,0x63,0x32,0x28,0x39,0x2e,0x39,0x39,
    0x39,0x39,0x39,0x39,0x39,0x37,0x34,0x37,0x35,0x32,0x34,0x32,0x37,0x30,0x37,0x38,
    0x37,0x38,0x33,0x35,0x31,0x32,0x31,0x31,0x35,0x34,0x37,0x38,0x35,0x65,0x2d,0x30,
    0x37,0x29,0x2c,0x20,0x76,0x65,0x63,0x32,0x28,0x30,0x2e,0x39,0x39,0x39,0x39,0x39,
    0x38,0x39,0x38,0x36,0x37,0x32,0x31,0x30,0x33,0x38,0x38,0x31,0x38,0x33,0x35,0x39,
    0x33,0x37,0x35,0x29,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x72,
    0x61,0x64,0x69,0x75,0x73,0x20,0x3d,0x20,0x5f,0x32,0x36,0x34,0x20,0x2b,0x20,0x5f,
    0x32,0x34,0x33,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x61,0x6e,0x67,
    0x6c,0x65,0x20,0x3d,0x20,0x5f,0x32,0x35,0x39,0x20,0x2b,0x20,0x33,0x2e,0x37,0x36,
    0x39,0x39,0x31,0x31,0x32,0x38,0x39,0x32,0x31,0x35,0x30,0x38,0x37,0x38,0x39,0x30,
    0x36,0x32,0x35,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,
    0x61,0x74,0x20,0x5f,0x32,0x38,0x31,0x20,0x3d,0x20,0x67,0x65,0x74,0x44,0x65,0x70,
    0x74,0x68,0x28,0x5f,0x32,0x37,0x32,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x6f,0x63,0x63,0x6c,0x75,0x73,0x69,0x6f,0x6e,0x53,0x75,0x6d,0x20,0x2b,
    0x3d,0x20,0x67,0x65,0x74,0x4f,0x63,0x63,0x6c,0x75,0x73,0x69,0x6f,0x6e,0x28,0x63,
    0x65,0x6e,0x74,0x65,0x72,0x56,0x69,0x65,0x77,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,
    0x6e,0x2c,0x20,0x5f,0x32,0x31,0x38,0x2c,0x20,0x67,0x65,0x74,0x56,0x69,0x65,0x77,
    0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x28,0x5f,0x32,0x37,0x32,0x2c,0x20,0x5f,
    0x32,0x38,0x31,0x20,0x2f,0x20,0x73,0x73,0x61,0x6f,0x5f,0x70,0x61,0x72,0x61,0x6d,
    0x73,0x5b,0x30,0x5d,0x2e,0x79,0x2c,0x20,0x67,0x65,0x74,0x56,0x69,0x65,0x77,0x5a,
    0x28,0x5f,0x32,0x38,0x31,0x29,0x29,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x7d,0x0a,
    0x20,0x20,0x20,0x20,0x72,0x65,0x74,0x75,0x72,0x6e,0x20,0x6f,0x63,0x63,0x6c,0x75,
    0x73,0x69,0x6f,0x6e,0x53,0x75,0x6d,0x20,0x2a,0x20,0x30,0x2e,0x32,0x30,0x30,0x30,
    0x30,0x30,0x30,0x30,0x32,0x39,0x38,0x30,0x32,0x33,0x32,0x32,0x33,0x38,0x37,0x36,
    0x39,0x35,0x33,0x31,0x32,0x35,0x3b,0x0a,0x7d,0x0a,0x0a,0x76,0x65,0x63,0x34,0x20,
    0x66,0x6c,0x6f,0x61,0x74,0x5f,0x74,0x6f,0x5f,0x72,0x67,0x62,0x61,0x28,0x66,0x6c,
    0x6f,0x61,0x74,0x20,0x76,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x76,0x65,0x63,
    0x34,0x20,0x5f,0x36,0x30,0x20,0x3d,0x20,0x66,0x72,0x61,0x63,0x74,0x28,0x76,0x65,
    0x63,0x34,0x28,0x31,0x2e,0x30,0x2c,0x20,0x32,0x35,0x35,0x2e,0x30,0x2c,0x20,0x36,
    0x35,0x30,0x32,0x35,0x2e,0x30,0x2c,0x20,0x31,0x36,0x30,0x35,0x38,0x31,0x33,0x37,
    0x36,0x2e,0x30,0x29,0x20,0x2a,0x20,0x76,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x72,
    0x65,0x74,0x75,0x72,0x6e,0x20,0x28,0x2d,0x5f,0x36,0x30,0x2e,0x79,0x7a,0x77,0x77,
    0x29,0x20,0x2a,0x20,0x76,0x65,0x63,0x34,0x28,0x30,0x2e,0x30,0x30,0x33,0x39,0x32,
    0x31,0x35,0x36,0x38,0x38,0x35,0x39,0x33,0x36,0x38,0x35,0x36,0x32,0x36,0x39,0x38,
    0x33,0x36,0x34,0x32,0x35,0x37,0x38,0x31,0x32,0x35,0x2c,0x20,0x30,0x2e,0x30,0x30,
    0x33,0x39,0x32,0x31,0x35,0x36,0x38,0x38,0x35,0x39,0x33,0x36,0x38,0x35,0x36,0x32,
    0x36,0x39,0x38,0x33,0x36,0x34,0x32,0x35,0x37,0x38,0x31,0x32,0x35,0x2c,0x20,0x30,
    0x2e,0x30,0x30,0x33,0x39,0x32,0x31,0x35,0x36,0x38,0x38,0x35,0x39,0x33,0x36,0x38,
    0x35,0x36,0x32,0x36,0x39,0x38,0x33,0x36,0x34,0x32,0x35,0x37,0x38,0x31,0x32,0x35,
    0x2c,0x20,0x30,0x2e,0x30,0x29,0x20,0x2b,0x20,0x5f,0x36,0x30,0x3b,0x0a,0x7d,0x0a,
    0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,
    0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x5f,0x33,0x30,0x38,0x20,0x3d,0x20,
    0x67,0x65,0x74,0x44,0x65,0x70,0x74,0x68,0x28,0x75,0x76,0x29,0x3b,0x0a,0x20,0x20,
    0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x5f,0x33,0x31,0x33,0x20,0x3d,0x20,0x5f,
    0x33,0x30,0x38,0x20,0x2f,0x20,0x73,0x73,0x61,0x6f,0x5f,0x70,0x61,0x72,0x61,0x6d,
    0x73,0x5b,0x30,0x5d,0x2e,0x79,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,
    0x74,0x20,0x70,0x61,0x72,0x61,0x6d,0x20,0x3d,0x20,0x5f,0x33,0x30,0x38,0x3b,0x0a,
    0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x5f,0x33,0x32,0x36,0x20,0x3d,
    0x20,0x67,0x65,0x74,0x41,0x6d,0x62,0x69,0x65,0x6e,0x74,0x4f,0x63,0x63,0x6c,0x75,
    0x73,0x69,0x6f,0x6e,0x28,0x67,0x65,0x74,0x56,0x69,0x65,0x77,0x50,0x6f,0x73,0x69,
    0x74,0x69,0x6f,0x6e,0x28,0x75,0x76,0x2c,0x20,0x5f,0x33,0x31,0x33,0x2c,0x20,0x67,
    0x65,0x74,0x56,0x69,0x65,0x77,0x5a,0x28,0x5f,0x33,0x30,0x38,0x29,0x29,0x2c,0x20,
    0x70,0x61,0x72,0x61,0x6d,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x72,0x61,0x67,
    0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x20,0x3d,0x20,0x28,0x66,0x6c,0x6f,0x61,0x74,0x5f,
    0x74,0x6f,0x5f,0x72,0x67,0x62,0x61,0x28,0x5f,0x33,0x32,0x36,0x29,0x20,0x2a,0x20,
    0x6d,0x61,0x78,0x28,0x30,0x2e,0x30,0x2c,0x20,0x30,0x2e,0x31,0x30,0x30,0x30,0x30,
    0x30,0x30,0x30,0x31,0x34,0x39,0x30,0x31,0x31,0x36,0x31,0x31,0x39,0x33,0x38,0x34,
    0x37,0x36,0x35,0x36,0x32,0x35,0x20,0x2d,0x20,0x5f,0x33,0x31,0x33,0x29,0x29,0x20,
    0x2a,0x20,0x31,0x30,0x2e,0x30,0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
#if !defined(SOKOL_GFX_INCLUDED)
  #error "Please include sokol_gfx.h before pp_ssao.h"
#endif
static inline const sg_shader_desc* ssao_shader_desc(sg_backend backend) {
  if (backend == SG_BACKEND_GLCORE33) {
    static sg_shader_desc desc;
    static bool valid;
    if (!valid) {
      valid = true;
      desc.attrs[0].name = "v_position";
      desc.attrs[1].name = "v_uv";
      desc.vs.source = ssao_vs_source_glsl330;
      desc.vs.entry = "main";
      desc.fs.source = ssao_fs_source_glsl330;
      desc.fs.entry = "main";
      desc.fs.uniform_blocks[0].size = 144;
      desc.fs.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
      desc.fs.uniform_blocks[0].uniforms[0].name = "ssao_params";
      desc.fs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
      desc.fs.uniform_blocks[0].uniforms[0].array_count = 9;
      desc.fs.images[0].used = true;
      desc.fs.images[0].multisampled = false;
      desc.fs.images[0].image_type = SG_IMAGETYPE_2D;
      desc.fs.images[0].sample_type = SG_IMAGESAMPLETYPE_FLOAT;
      desc.fs.samplers[0].used = true;
      desc.fs.samplers[0].sampler_type = SG_SAMPLERTYPE_FILTERING;
      desc.fs.image_sampler_pairs[0].used = true;
      desc.fs.image_sampler_pairs[0].image_slot = 0;
      desc.fs.image_sampler_pairs[0].sampler_slot = 0;
      desc.fs.image_sampler_pairs[0].glsl_name = "t_depth_t_smp";
      desc.label = "ssao_shader";
    }
    return &desc;
  }
  return 0;
}
