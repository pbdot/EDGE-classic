 @vs vs
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

    @fs fs
    uniform texture2D tex;
    uniform sampler smp;
    in vec4 uv;
    in vec4 color;
    out vec4 frag_color;
    void main() {
        
        frag_color = texture(sampler2D(tex, smp), uv.xy) * color;

        if (floor(frag_color.a*255.) <= 16) discard;
    }
    @end

    @program world vs fs