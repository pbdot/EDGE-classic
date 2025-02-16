
@vs ssao_vs
@hlsl_options fixup_clipspace flip_vert_y
in vec4 v_position;
in vec2 v_uv;
out vec2 TexCoord;
void main() {
  gl_Position = v_position;
  TexCoord = v_uv;
}
@end

@fs ssao_fs

layout(location=0) in vec2 TexCoord;
layout(location=0) out vec4 FragColor;

@image_sample_type DepthTexture unfilterable_float
layout(binding=0) uniform texture2D DepthTexture;
@sampler_type DepthSampler nonfiltering
layout(binding=0) uniform sampler DepthSampler;

layout(binding=1) uniform texture2D NormalTexture;
layout(binding=1) uniform sampler NormalSampler;

layout(binding=2) uniform texture2D RandomTexture;
layout(binding=2) uniform sampler RandomSampler;

layout(binding=0) uniform ssao_params {
    vec2 UVToViewA;
    vec2 UVToViewB;
    vec2 InvFullResolution;
    float NDotVBias;
    float NegInvR2;
    float RadiusToScreen;
    float AOMultiplier;
    float AOStrength;
    int SampleIndex;
    float Padding0;
    float Padding1;
    vec2 Scale;
    vec2 Offset;
};

#define PI 3.14159265358979323846
// High Quality
#define NUM_DIRECTIONS 8.0
#define NUM_STEPS 4.0

// Calculate eye space position for the specified texture coordinate
vec3 FetchViewPos(vec2 uv)
{
	float z = textureLod(sampler2D(DepthTexture, DepthSampler), uv, 0.0).x;
    return vec3((UVToViewA * uv + UVToViewB) * z, z);
}

vec3 SampleNormal(vec2 uv)
{
	return texture(sampler2D(NormalTexture, NormalSampler) ,uv, 0).xyz * 2.0 - 1.0;
}

// Look up the eye space normal for the specified texture coordinate
vec3 FetchNormal(vec2 uv)
{
	vec3 normal = SampleNormal(Offset + uv * Scale);
	if (length(normal) > 0.1)
	{
		normal = normalize(normal);
		normal.z = -normal.z;
		return normal;
	}
	else
	{
		return vec3(0.0);
	}
}

// Compute normalized 2D direction
vec2 RotateDirection(vec2 dir, vec2 cossin)
{
    return vec2(dir.x * cossin.x - dir.y * cossin.y, dir.x * cossin.y + dir.y * cossin.x);
}

#define RANDOM_TEXTURE_WIDTH 4.0

vec4 GetJitter()
{    
    return texture(sampler2D(RandomTexture, RandomSampler), gl_FragCoord.xy / RANDOM_TEXTURE_WIDTH, 0);
}

// Calculates the ambient occlusion of a sample
float ComputeSampleAO(vec3 kernelPos, vec3 normal, vec3 samplePos)
{
	vec3 v = samplePos - kernelPos;
	float distanceSquare = dot(v, v);
	float nDotV = dot(normal, v) * inversesqrt(distanceSquare);
	return clamp(nDotV - NDotVBias, 0.0, 1.0) * clamp(distanceSquare * NegInvR2 + 1.0, 0.0, 1.0);
}

// Calculates the total ambient occlusion for the entire fragment
float ComputeAO(vec3 viewPosition, vec3 viewNormal)
{
    vec4 rand = GetJitter();

	float radiusPixels = RadiusToScreen / viewPosition.z;
	float stepSizePixels = radiusPixels / (NUM_STEPS + 1.0);

	const float directionAngleStep = 2.0 * PI / NUM_DIRECTIONS;
	float ao = 0.0;

    for (float directionIndex = 0.0; directionIndex < NUM_DIRECTIONS; ++directionIndex)
    {
        float angle = directionAngleStep * directionIndex;

        vec2 direction = RotateDirection(vec2(cos(angle), sin(angle)), rand.xy);
        float rayPixels = (rand.z * stepSizePixels + 1.0);

        for (float StepIndex = 0.0; StepIndex < NUM_STEPS; ++StepIndex)
        {
            vec2 sampleUV = round(rayPixels * direction) * InvFullResolution + TexCoord;
            vec3 samplePos = FetchViewPos(sampleUV);
            ao += ComputeSampleAO(viewPosition, viewNormal, samplePos);
            rayPixels += stepSizePixels;
        }
    }

    ao *= AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
    return clamp(1.0 - ao * 2.0, 0.0, 1.0);
}

void main()
{
    vec3 viewPosition = FetchViewPos(TexCoord);
	vec3 viewNormal = FetchNormal(TexCoord);
	float occlusion = viewNormal != vec3(0.0) ? ComputeAO(viewPosition, viewNormal) * AOStrength + (1.0 - AOStrength) : 1.0;

	FragColor = vec4(occlusion, viewPosition.z, 0.0, 1.0);    
}
@end

@program ssao ssao_vs ssao_fs
