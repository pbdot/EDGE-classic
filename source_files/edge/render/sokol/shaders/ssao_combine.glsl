
@vs ssao_combine_vs
@hlsl_options fixup_clipspace flip_vert_y
in vec4 v_position;
in vec2 v_uv;
out vec2 TexCoord;
void main() {
  gl_Position = v_position;
  TexCoord = v_uv;
}
@end


@fs ssao_combine_fs

layout(location=0) in vec2 TexCoord;
layout(location=0) out vec4 FragColor;

@image_sample_type AODepthTexture unfilterable_float
layout(binding=0) uniform texture2D AODepthTexture;

@sampler_type AODepthSampler nonfiltering
layout(binding=0) uniform sampler AODepthSampler;

//@image_sample_type SceneFogTexture unfilterable_float
layout(binding=1) uniform texture2D SceneFogTexture;

//@sampler_type SceneFogSampler nonfiltering
layout(binding=1) uniform sampler SceneFogSampler;

#define AODepthTexture_ sampler2D(AODepthTexture, AODepthSampler)
#define SceneFogTexture_ sampler2D(SceneFogTexture, SceneFogSampler)

void main()
{
	vec4 fogColor = texture(SceneFogTexture_, TexCoord);
	vec4 ssao = texture(AODepthTexture_, TexCoord);
	float attenutation = ssao.x;

    FragColor = fogColor * vec4(ssao.xxx, 1.0) ;

    //FragColor = vec4(fogColor, 1.0 - attenutation);
/*
	if (DebugMode == 0)
		FragColor = vec4(fogColor, 1.0 - attenutation);
	else if (DebugMode < 3)
		FragColor = vec4(attenutation, attenutation, attenutation, 1.0);
	else if (DebugMode == 3)
		FragColor = vec4(ssao.yyy / 1000.0, 1.0);
	else
		FragColor = vec4(ssao.xyz, 1.0);
*/        
}

@end

@program ssao_combine ssao_combine_vs ssao_combine_fs
