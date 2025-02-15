
@vs blur_vs
in vec4 v_position;
in vec2 v_uv;
out vec2 TexCoord;
void main() {
  gl_Position = v_position;
  TexCoord = v_uv;
}
@end


@fs blur_fs

#define SAMPLES 64.0

layout(location=0) in vec2 TexCoord;
layout(location=0) out vec4 FragColor;

@image_sample_type SourceTexture unfilterable_float
layout(binding=0) uniform texture2D SourceTexture;

@sampler_type SourceSampler nonfiltering
layout(binding=0) uniform sampler SourceSampler;

#define SourceTexture_ sampler2D(SourceTexture, SourceSampler)

layout(binding=0) uniform blur_params {
    int direction;
    float SampleWeights0;
    float SampleWeights1;
    float SampleWeights2;
    float SampleWeights3;
    float SampleWeights4;
    float SampleWeights5;
    float SampleWeights6;
};

void main()
{
    if (direction == 0)
    {
        FragColor =
            textureOffset(SourceTexture_, TexCoord, ivec2( 0, 0)) * SampleWeights0 +
            textureOffset(SourceTexture_, TexCoord, ivec2( 1, 0)) * SampleWeights1 +
            textureOffset(SourceTexture_, TexCoord, ivec2(-1, 0)) * SampleWeights2 +
            textureOffset(SourceTexture_, TexCoord, ivec2( 2, 0)) * SampleWeights3 +
            textureOffset(SourceTexture_, TexCoord, ivec2(-2, 0)) * SampleWeights4 +
            textureOffset(SourceTexture_, TexCoord, ivec2( 3, 0)) * SampleWeights5 +
            textureOffset(SourceTexture_, TexCoord, ivec2(-3, 0)) * SampleWeights6;
    }
    else
    {
        FragColor =
            textureOffset(SourceTexture_, TexCoord, ivec2(0, 0)) * SampleWeights0 +
            textureOffset(SourceTexture_, TexCoord, ivec2(0, 1)) * SampleWeights1 +
            textureOffset(SourceTexture_, TexCoord, ivec2(0,-1)) * SampleWeights2 +
            textureOffset(SourceTexture_, TexCoord, ivec2(0, 2)) * SampleWeights3 +
            textureOffset(SourceTexture_, TexCoord, ivec2(0,-2)) * SampleWeights4 +
            textureOffset(SourceTexture_, TexCoord, ivec2(0, 3)) * SampleWeights5 +
            textureOffset(SourceTexture_, TexCoord, ivec2(0,-3)) * SampleWeights6;
    }

}
@end

@program blur blur_vs blur_fs
