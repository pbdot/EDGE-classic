

@vs depth_vs

uniform depth_vs_params {
    mat4 mvp;        
};

in vec4 v_pos;
out vec3 position;
void main() {
    gl_Position = mvp * v_pos;      
    position = gl_Position.xyz;
}

@end

@fs depth_fs

uniform depth_fs_params
{    
    uniform float u_far;    
};

in vec3 position;
out vec4 frag_color;

vec4 float_to_rgba(const in float v) {
  vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;
  enc = fract(enc);
  enc -= enc.yzww * vec4(1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0, 0.0);
  return enc;
}

/*
vec4 depth_to_rgba(vec3 pos) {
  vec3 pos_norm = pos / u_far;
  float d = length(pos_norm) * u_far;
  d = clamp(d, 0.0, u_far * 0.98);
  d = log(0.05 * d + 1.0) / log(0.05 * u_far + 1.0);
  return float_to_rgba(d);
}
*/

vec4 depth_to_rgba(vec3 pos) {
  vec3 pos_norm = pos / u_far;
  float d = length(pos_norm);
  d = clamp(d, 0.0, 1.0);
  return float_to_rgba(d);
}


void main() {
    frag_color = depth_to_rgba(position);        
}
@end

@program depth depth_vs depth_fs