
@vs linear_depth_vs
@hlsl_options fixup_clipspace flip_vert_y
in vec4 v_position;
in vec2 v_uv;
out vec2 uv;
void main() {
  gl_Position = v_position;
  uv = v_uv;
}
@end

@fs linear_depth_fs

layout(location=0) in vec2 uv;
layout(location=0) out vec4 frag_color;

@image_sample_type depth_texture unfilterable_float
layout(binding=0) uniform texture2D depth_texture;

@sampler_type depth_sampler nonfiltering
layout(binding=0) uniform sampler depth_sampler;

//@image_sample_type color_texture unfilterable_float
layout(binding=1) uniform texture2D color_texture;

//@sampler_type color_sampler nonfiltering
layout(binding=1) uniform sampler color_sampler;

layout(binding=0) uniform linear_depth_params {
    float inverse_depth_range_a;
    float inverse_depth_range_b;
    float linearize_depth_a;
    float linearize_depth_b;    
};

float normalizeDepth(float depth)
{
	float normalized_depth = clamp(inverse_depth_range_a * depth + inverse_depth_range_b, 0.0, 1.0);
	return 1.0 / (normalized_depth * linearize_depth_a + linearize_depth_b);
}

void main()
{
	//float depth = normalizeDepth(texelFetch(color_texture, ipos, 0).a != 0.0 ? texelFetch(depth_texture, ipos, 0).x : 1.0);

  vec2 uvpos = max(uv, vec2(0.0));

    float depth = normalizeDepth(texture(sampler2D(color_texture, color_sampler), uvpos, 0).a != 0.0 ? texture(sampler2D(depth_texture, depth_sampler), uvpos, 0).x : 1.0);    
	frag_color = vec4(depth, 0.0, 0.0, 1.0);
}
@end

@program linear_depth linear_depth_vs linear_depth_fs
