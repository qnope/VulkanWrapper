// Random Sampling - Shared definitions for hemisphere sampling
// This file provides random sampling functions using precomputed samples
// and a noise texture for per-pixel decorrelation (Cranley-Patterson rotation).
//
// Usage: Define bindings before including this file:
//
//   #define RANDOM_XI_BUFFER_BINDING 8
//   #define RANDOM_NOISE_TEXTURE_BINDING 9
//   #include "random.glsl"
//
// The Xi buffer should contain DualRandomSample (4096 vec2 values in [0,1)).
// The noise texture should be a 4096x4096 RG32F texture with random values.

#ifndef RANDOM_GLSL
#define RANDOM_GLSL

// Verify required bindings are defined
#ifndef RANDOM_XI_BUFFER_BINDING
#error "RANDOM_XI_BUFFER_BINDING must be defined before including random.glsl"
#endif

#ifndef RANDOM_NOISE_TEXTURE_BINDING
#error "RANDOM_NOISE_TEXTURE_BINDING must be defined before including random.glsl"
#endif

// Constants
const uint DUAL_SAMPLE_COUNT = 4096;
const uint NOISE_TEXTURE_SIZE = 4096;
const float RANDOM_PI = 3.14159265359;

// Storage buffer containing precomputed hemisphere sample pairs
layout(set = 0, binding = RANDOM_XI_BUFFER_BINDING, std430) readonly buffer DualRandomSampleBuffer {
    vec2 xi_samples[DUAL_SAMPLE_COUNT];
};

// Noise texture for per-pixel decorrelation (RG32F format, repeat wrapping)
layout(set = 0, binding = RANDOM_NOISE_TEXTURE_BINDING) uniform sampler2D noise_texture;

// Get a random sample for the given index and pixel using Cranley-Patterson rotation.
// Returns fract(xi[sample_index] + noise_texture[pixel])
// This decorrelates neighboring pixels while maintaining low-discrepancy properties.
vec2 get_sample(uint sample_index, ivec2 pixel) {
    uint idx = sample_index % DUAL_SAMPLE_COUNT;
    vec2 xi = xi_samples[idx];
    vec2 noise_uv = vec2(pixel) / float(NOISE_TEXTURE_SIZE);
    vec2 noise = texture(noise_texture, noise_uv).rg;
    return fract(xi + noise);
}

// Cosine-weighted hemisphere sampling using TBN basis.
// N = normal, T = tangent, B = bitangent
// xi = random sample pair in [0, 1)
// Returns direction in world space.
vec3 sample_hemisphere_cosine(vec3 N, vec3 T, vec3 B, vec2 xi) {
    float phi = 2.0 * RANDOM_PI * xi.x;
    float cos_theta = sqrt(1.0 - xi.y);
    float sin_theta = sqrt(xi.y);

    // Direction in tangent space
    vec3 local = vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);

    // Transform to world space using TBN
    return normalize(T * local.x + B * local.y + N * local.z);
}

// PDF for cosine-weighted hemisphere sampling.
// cos_theta = dot(direction, normal)
// Returns cos_theta / PI.
float pdf_hemisphere_cosine(float cos_theta) {
    return cos_theta / RANDOM_PI;
}

#endif // RANDOM_GLSL
