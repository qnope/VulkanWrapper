#ifndef GBUFFER_BASE_GLSL
#define GBUFFER_BASE_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_ray_query : require
#extension GL_GOOGLE_include_directive : require

#ifdef GBUFFER_TEXTURED
#extension GL_EXT_nonuniform_qualifier : require
#endif

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 tangeant;
layout(location = 2) in vec3 biTangeant;
layout(location = 3) in vec2 texCoord;
layout(location = 4) in vec3 worldPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangeant;
layout(location = 3) out vec3 outBiTangeant;
layout(location = 4) out vec4 outPosition;
layout(location = 5) out vec4 outDirectLight;
layout(location = 6) out vec4 outIndirectRay;

// Random sampling with Cranley-Patterson rotation
#define RANDOM_XI_BUFFER_BINDING 1
#define RANDOM_NOISE_TEXTURE_BINDING 2
#include "random.glsl"

// Atmosphere parameters (must be before UBO declaration)
#include "atmosphere_params.glsl"

// Sky parameters UBO
layout(set = 0, binding = 3, std140) uniform SkyParamsUBO {
    SkyParameters sky;
};

// TLAS for shadow ray queries
layout(set = 0, binding = 4) uniform accelerationStructureEXT topLevelAS;

// Atmosphere scattering and sun lighting
// (must be after SkyParameters is accessible)
#include "atmosphere_scattering.glsl"
#include "sun_lighting_computation.glsl"

#ifdef GBUFFER_TEXTURED
// Bindless texture descriptor set
layout(set = 1, binding = 0) uniform sampler globalSampler;
layout(set = 1, binding = 1) uniform texture2D textures[];

#define BRDF_SAMPLER globalSampler
#define BRDF_HAS_QUERY_LOD
vec2 _brdf_uv;
#endif

layout(push_constant, scalar) uniform PushConstants {
    mat4 model;
    uint64_t materialAddress;
    uint frame_count;
};

// Forward declaration - implemented by material BRDF include
vec3 evaluate_brdf(vec3 normal, uint64_t material_address,
                   vec3 wi, vec3 wo);

void main()
{
#ifdef GBUFFER_TEXTURED
    _brdf_uv = texCoord;
#endif

    vec3 N = normalize(normal);
    vec3 brdf = evaluate_brdf(N, materialAddress, vec3(0), vec3(0));

    outColor = vec4(brdf * ATMO_PI, 1.0);
    outNormal = normal;
    outTangeant = tangeant;
    outBiTangeant = biTangeant;
    outPosition = vec4(worldPosition, 1.0);

    // Compute direct sun lighting with shadow rays
    vec3 sun = luminance_from_sun(sky, worldPosition, N,
                                   topLevelAS);
    outDirectLight = vec4(brdf * sun, 1.0);

    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 xi = get_sample(frame_count, pixel);
    vec3 T = normalize(tangeant);
    vec3 B = normalize(biTangeant);
    vec3 ray_dir = sample_hemisphere_cosine(N, T, B, xi);
    outIndirectRay = vec4(ray_dir, 1.0);
}

#endif // GBUFFER_BASE_GLSL
