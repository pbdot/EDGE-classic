
@vs depthblur_vs
in vec4 v_position;
in vec2 v_uv;
out vec2 TexCoord;
void main() {
  gl_Position = v_position;
  TexCoord = v_uv;
}
@end


@fs depthblur_fs

layout(location=0) in vec2 TexCoord;
layout(location=0) out vec4 FragColor;

@image_sample_type AODepthTexture unfilterable_float
layout(binding=0) uniform texture2D AODepthTexture;

@sampler_type AODepthSampler nonfiltering
layout(binding=0) uniform sampler AODepthSampler;

layout(binding=0) uniform depthblur_params {
    int direction;
	float BlurSharpness;
	float PowExponent;
};

#define AODepthTexture_ sampler2D(AODepthTexture, AODepthSampler)

#define KERNEL_RADIUS 3.0

void AddSample(vec2 blurSample, float r, float centerDepth, inout float totalAO, inout float totalW)
{
	const float blurSigma = KERNEL_RADIUS * 0.5;
	const float blurFalloff = 1.0 / (2.0 * blurSigma * blurSigma);

	float ao = blurSample.x;
	float z = blurSample.y;

	float deltaZ = (z - centerDepth) * BlurSharpness;
	float w = exp2(-r * r * blurFalloff - deltaZ * deltaZ);

	totalAO += w * ao;
	totalW += w;
}

void main()
{
	vec2 centerSample = textureOffset(AODepthTexture_, TexCoord, ivec2( 0, 0)).xy;
	float centerDepth = centerSample.y;
	float totalAO = centerSample.x;
	float totalW = 1.0;

if (direction == 0)
{
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2(-3, 0)).xy, 3.0, centerDepth, totalAO, totalW);
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2(-2, 0)).xy, 2.0, centerDepth, totalAO, totalW);
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2(-1, 0)).xy, 1.0, centerDepth, totalAO, totalW);
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2( 1, 0)).xy, 1.0, centerDepth, totalAO, totalW);
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2( 2, 0)).xy, 2.0, centerDepth, totalAO, totalW);
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2( 3, 0)).xy, 3.0, centerDepth, totalAO, totalW);
}
else
{
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2(0, -3)).xy, 3.0, centerDepth, totalAO, totalW);
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2(0, -2)).xy, 2.0, centerDepth, totalAO, totalW);
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2(0, -1)).xy, 1.0, centerDepth, totalAO, totalW);
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2(0,  1)).xy, 1.0, centerDepth, totalAO, totalW);
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2(0,  2)).xy, 2.0, centerDepth, totalAO, totalW);
	AddSample(textureOffset(AODepthTexture_, TexCoord, ivec2(0,  3)).xy, 3.0, centerDepth, totalAO, totalW);
}

	float fragAO = totalAO / totalW;

    if (direction == 0)
    {
        FragColor = vec4(fragAO, centerDepth, 0.0, 1.0);
    }
    else
    {
        FragColor = vec4(pow(clamp(fragAO, 0.0, 1.0), PowExponent), 0.0, 0.0, 1.0);
    }

}
@end

@program depthblur depthblur_vs depthblur_fs
