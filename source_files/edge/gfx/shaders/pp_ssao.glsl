
/*
  // pass through
  "layout(location=0) in vec4 v_position;\n"
  "layout(location=1) in vec2 v_uv;\n"
  "out vec2 uv;\n"
  "void main() {\n"
  "  gl_Position = v_position;\n"
  "  uv = v_uv;\n"
  "}\n";
*/

@vs ssao_vs
in vec4 v_position;
in vec2 v_uv;
out vec2 uv;
void main() {
  gl_Position = v_position;
  uv = v_uv;
}
@end

@fs ssao_fs

#define PI 3.1415926535897932384626433832795
#define PI2 (2.0 * PI)
#define EPSILON 1e-6f

out vec4 frag_color;
in vec2 uv;
uniform ssao_params {
  // common
  float u_near;
  float u_far;
  vec2 u_target_size;
  mat4 u_mat_p;
  mat4 u_inv_mat_p;
};

// ssao  
uniform texture2D t_depth;
uniform sampler t_smp;

vec4 float_to_rgba(const in float v) {
  vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;
  enc = fract(enc);
  enc -= enc.yzww * vec4(1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0, 0.0);
  return enc;
}

float rgba_to_float(const in vec4 rgba) {
  return dot(rgba, vec4(1.0, 1.0 / 255.0, 1.0 / 65025.0, 1.0 / 160581375.0));
}

float pow2(const in float v) {
  return v * v;
}

float rgba_to_depth(vec4 rgba) {
  float d = rgba_to_float(rgba);
  d *= log(0.05 * u_far + 1.0);
  d = exp(d);
  d -= 1.0;
  d /= 0.05;
  return d;
}

float rgba_to_depth_log(vec4 rgba) {
  return rgba_to_float(rgba);
}

highp float rand( const in vec2 uv ) {
    const highp float a = 12.9898, b = 78.233, c = 43758.5453;
    highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
    return fract( sin( sn ) * c );
}


// Based on https://threejs.org/examples/webgl_postprocessing_sao.html

// Increase/decrease to trade quality for performance
#define NUM_SAMPLES 5
// The sample kernel uses a spiral pattern so most samples are concentrated
// close to the center. 
#define NUM_RINGS 3
#define KERNEL_RADIUS 15.0
// Misc params, tweaked to match the renderer
#define BIAS 0.2
#define SCALE 1.0
// Derived constants
#define ANGLE_STEP ((PI2 * float(NUM_RINGS)) / float(NUM_SAMPLES))
#define INV_NUM_SAMPLES (1.0 / float(NUM_SAMPLES))

float getDepth(const in vec2 t_uv) {
    return rgba_to_depth(texture(sampler2D(t_depth, t_smp), t_uv));
}

float getViewZ(const in float depth) {
    return (u_near * u_far) / (depth - u_far);
}

// Compute position in world space from depth & projection matrix
vec3 getViewPosition( const in vec2 screenPosition, const in float depth, const in float viewZ ) {
    float clipW = u_mat_p[2][3] * viewZ + u_mat_p[3][3];
    vec4 clipPosition = vec4( ( vec3( screenPosition, depth ) - 0.5 ) * 2.0, 1.0 );
    clipPosition *= clipW; // unprojection.
    return ( u_inv_mat_p * clipPosition ).xyz;
}

// Compute normal from derived position. Should at some point replace it
// with reading from a normal buffer so it works correctly with smooth
// shading / normal maps.
vec3 getViewNormal( const in vec3 viewPosition, const in vec2 t_uv ) {
    return normalize( cross( dFdx( viewPosition ), dFdy( viewPosition ) ) );
}

float scaleDividedByCameraFar;

// Compute occlusion of single sample
float getOcclusion( const in vec3 centerViewPosition, const in vec3 centerViewNormal, const in vec3 sampleViewPosition ) {
    vec3 viewDelta = sampleViewPosition - centerViewPosition;
    float viewDistance = length( viewDelta );
    float scaledScreenDistance = scaleDividedByCameraFar * viewDistance;
    float n_dot_d = dot(centerViewNormal, viewDelta);
    float scaled_n_dot_d = max(0.0, n_dot_d / scaledScreenDistance - BIAS);
    float result = scaled_n_dot_d / (1.0 + pow2(scaledScreenDistance));

    // Strip off values that are too large which eliminates shadowing objects
    // that are far away.
    if (result > 220.0) {
      result = 0.0;
    }

    // Squash the range and offset noise.
    return max(0.0, clamp(result, 1.1, 20.0) / 13.0 - 0.2);
}

float getAmbientOcclusion( const in vec3 centerViewPosition, float centerDepth ) {
  scaleDividedByCameraFar = SCALE / u_far;
  vec3 centerViewNormal = getViewNormal( centerViewPosition, uv );

  float angle = rand( uv ) * PI2;
  vec2 radius = vec2( KERNEL_RADIUS * INV_NUM_SAMPLES );

  // Use smaller kernels for objects farther away from the camera
  radius /= u_target_size * centerDepth * 0.05;
  // Make sure tha the sample radius isn't less than a single texel, as this
  // introduces noise
  radius = max(radius, 5.0 / u_target_size);

  vec2 radiusStep = radius;
  float occlusionSum = 0.0;

  // Collect occlusion samples
  for( int i = 0; i < NUM_SAMPLES; i ++ ) {
    vec2 sampleUv = uv + vec2( cos( angle ), sin( angle ) ) * radius;

    // Don't sample outside of texture coords to prevent edge artifacts
    sampleUv = clamp(sampleUv, EPSILON, 1.0 - EPSILON);

    radius += radiusStep;
    angle += ANGLE_STEP;

    float sampleDepth = getDepth( sampleUv );
    float sampleDepthNorm = sampleDepth / u_far;

    float sampleViewZ = getViewZ( sampleDepth );
    vec3 sampleViewPosition = getViewPosition( sampleUv, sampleDepthNorm, sampleViewZ );
    occlusionSum += getOcclusion( centerViewPosition, centerViewNormal, sampleViewPosition );
  }

  return occlusionSum * (1.0 / (float(NUM_SAMPLES)));
}

void main() {
  // Depth (range = 0 .. u_far)
  float centerDepth = getDepth( uv );

  // Depth (range = 0 .. 1)
  float centerDepthNorm = centerDepth / u_far;
  float centerViewZ = getViewZ( centerDepth );
  vec3 viewPosition = getViewPosition( uv, centerDepthNorm, centerViewZ );
  float ambientOcclusion = getAmbientOcclusion( viewPosition, centerDepth );

  // Store value as rgba to increase precision
  float max_dist = 0.1;
  float mult = 1.0 / max_dist;
  frag_color = float_to_rgba(ambientOcclusion) * max(0.0, max_dist - centerDepthNorm) * mult;  

  //frag_color.x = 1;
}

@end

@program ssao ssao_vs ssao_fs