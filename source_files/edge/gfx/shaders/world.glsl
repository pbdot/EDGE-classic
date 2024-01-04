 @vs world_vs
    uniform vs_params {
        mat4 mvp;        
    };
    in vec4 color0;
    in vec4 position;
    in vec4 texcoords;        
    out vec4 uv;
    out vec4 color;
    void main() {
        gl_Position = mvp * position;      
         uv = texcoords;
        color = color0;
    }
    @end

    @fs world_fs
    uniform texture2D tex0;
    uniform sampler smp0;
    uniform texture2D tex1;    
    uniform sampler smp1;
    in vec4 uv;
    in vec4 color;
    out vec4 frag_color;
    void main() {
        
        
        vec4 c0 = texture(sampler2D(tex0, smp0), uv.xy) * color;
        if (floor(c0.a*255) <= 16) discard;

        vec4 c1 = texture(sampler2D(tex1, smp1), uv.zw);
        c0.rgb *= c1.rgb;
        c0.rgb = clamp(c0.rgb, 0., 1.);
        frag_color = c0;

        
    }
    @end

    @program world world_vs world_fs