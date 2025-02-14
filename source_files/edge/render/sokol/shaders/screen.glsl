

@vs screen_vs
in vec4 v_position;
in vec2 v_uv;
out vec2 uv;
void main() {
  gl_Position = v_position;
  uv = v_uv;
}
@end

@fs screen_fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;
out vec4 frag_color;
in vec2 uv;

void main() {
    frag_color = texture(sampler2D(tex,smp), uv);
}
@end

@program screen screen_vs screen_fs