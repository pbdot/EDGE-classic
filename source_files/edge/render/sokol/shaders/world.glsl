@ctype mat4 HMM_Mat4

@vs vs
layout(binding=0) uniform vs_params {
    mat4 mvp;
    mat4 tm;
    mat4 mv;
};
layout(location = 0) in vec4 position; 
layout(location = 1) in vec4 texcoords; 
layout(location = 2) in vec4 color0; 
layout(location = 3) in float psize; 
layout(location = 0) out vec4 uv; 
layout(location = 1) out vec4 color; 
layout(location = 2) out float fog_src; 

void main() 
{     
    vec4 vertex = mv * position;
    gl_Position = mvp * position; 
    gl_PointSize = psize; 
    uv = texcoords; 
    // FIXME: texture matrix currently disabled
    //uv = tm * vec4(texcoords.xy, 0.0, 1.0);
    color = color0; 
    fog_src = vertex.z;
}
@end

@fs fs
layout(binding=1) uniform state {
    int flags;
    vec4 fog_color;
    float fog_density;
    float fog_start;
    float fog_end;
    float fog_scale;
};

layout(binding=0) uniform texture2D tex0;
layout(binding=0) uniform sampler smp0;
layout(binding=1) uniform texture2D tex1;
layout(binding=1) uniform sampler smp1;

layout(location = 0) out vec4 frag_color;
layout(location = 0) in vec4 uv;
layout(location = 1) in vec4 color;
layout(location = 2) in float fog_src;

void main()
{
    vec4 c0 = texture(sampler2D(tex0, smp0), uv.xy) * color;
    if (floor(c0.w * 255.0) <= 0.5)
    {
        discard;
    }
    if ((flags & 1) == 1)
    {
        c0.rgb *= texture(sampler2D(tex1, smp1), uv.zw).rgb;
    }
    
    if ((flags & 2) == 2)
    {
        float fog_c = abs(fog_src);
        float fogf = clamp(exp(-fog_density * fog_c), 0., 1.);
        c0.rgb = mix(fog_color.rgb, c0.rgb, fogf);
    }
    c0.rgb = clamp(c0.rgb, vec3(0), vec3(1));    
    frag_color = c0;
}
@end

@program sgl vs fs

