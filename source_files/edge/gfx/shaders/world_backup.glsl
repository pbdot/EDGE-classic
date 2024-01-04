    @vs vs
    uniform vs_params {
        mat4 mvp;
        mat4 tm;
    };
    in vec4 position;
    in vec2 texcoord0;
    in vec4 color0;
    out vec4 uv;
    out vec4 color;
    void main() {
        gl_Position = mvp * position;
        uv = tm * vec4(texcoord0, 0.0, 1.0);
        color = color0;
    }
    @end

    @fs fs
    uniform state {
        int alphaFunc;
        float alphaRef;
    };

    uniform texture2D tex;
    uniform sampler smp;
    in vec4 uv;
    in vec4 color;
    out vec4 frag_color;
    void main() {

        vec4 tcolor = texture(sampler2D(tex, smp), uv.xy) * color;

        if (alphaFunc == 1)
        {
            if (floor(tcolor.a*255.) <= alphaFunc) discard;
        }

        frag_color = tcolor;
    }
    @end

    @program world vs fs
