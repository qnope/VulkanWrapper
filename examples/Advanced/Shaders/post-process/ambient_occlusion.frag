#version 460
#extension GL_EXT_ray_query : require
#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D samplerPosition;
layout (set = 0, binding = 1) uniform sampler2D samplerNormal;
layout (set = 0, binding = 2) uniform sampler2D samplerTangent;
layout (set = 0, binding = 3) uniform sampler2D samplerBitangent;
layout (set = 0, binding = 4) uniform accelerationStructureEXT topLevelAS;

// Random sampling with Cranley-Patterson rotation
#define RANDOM_XI_BUFFER_BINDING 5
#define RANDOM_NOISE_TEXTURE_BINDING 6
#include "random.glsl"

layout (push_constant) uniform PushConstants {
    float aoRadius;
    int sampleIndex; // Which sample to use this frame (for progressive accumulation)
} pushConstants;

void main()
{
    vec4 positionSample = texture(samplerPosition, inUV);

    // Skip background (alpha = 0 means no geometry was rendered)
    // Output white (no occlusion) for background pixels
    if (positionSample.a < 0.5) {
        outColor = vec4(1.0);
        return;
    }

    vec3 position = positionSample.rgb;
    vec3 normal = normalize(texture(samplerNormal, inUV).rgb);
    vec3 tangent = normalize(texture(samplerTangent, inUV).rgb);
    vec3 bitangent = normalize(texture(samplerBitangent, inUV).rgb);

    float aoRadius = pushConstants.aoRadius;

    // Get random sample using Cranley-Patterson rotation for per-pixel decorrelation
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 xi = get_sample(uint(pushConstants.sampleIndex), pixel);

    // Generate cosine-weighted direction in hemisphere around normal
    vec3 sampleDir = sample_hemisphere_cosine(normal, tangent, bitangent, xi);

    // Offset ray origin along normal to avoid self-intersection
    vec3 rayOrigin = position + normal * 0.01;

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(
        rayQuery,
        topLevelAS,
        gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT,
        0xFF,
        rayOrigin,
        0.0,
        sampleDir,
        aoRadius
    );

    // Traverse the acceleration structure
    while (rayQueryProceedEXT(rayQuery)) {
        // For opaque geometry, no processing needed
    }

    // Check if we hit anything (1.0 = occluded, 0.0 = visible)
    float occlusion = 0.0;
    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
        occlusion = 1.0;
    }

    // Convert to visibility (1 = fully visible, 0 = fully occluded)
    // Hardware blending will accumulate this with previous frames
    float visibility = 1.0 - occlusion;

    // Output greyscale RGBA
    outColor = vec4(visibility, visibility, visibility, 1.0);
}
