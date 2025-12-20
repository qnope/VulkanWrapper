#version 460
#extension GL_EXT_ray_query : require

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D samplerPosition;
layout (set = 0, binding = 1) uniform sampler2D samplerNormal;
layout (set = 0, binding = 2) uniform sampler2D samplerTangent;
layout (set = 0, binding = 3) uniform sampler2D samplerBitangent;
layout (set = 0, binding = 4) uniform accelerationStructureEXT topLevelAS;

// Maximum number of samples supported
const int MAX_SAMPLES = 1024;

// UBO containing random values for cosine-weighted sampling
layout (set = 0, binding = 5) uniform AOSamples {
    // xi1 is used for azimuthal angle (phi = 2*PI*xi1)
    // xi2 is used for polar angle via cosine-weighted distribution (cos(theta) = sqrt(xi2))
    vec4 samples[MAX_SAMPLES]; // xy = (xi1, xi2) for each sample
} aoSamples;

layout (push_constant) uniform PushConstants {
    float aoRadius;
    int sampleIndex; // Which sample to use this frame (0 to MAX_SAMPLES-1)
} pushConstants;

// Compute cosine-weighted hemisphere direction from random values
// Uses the standard cosine-weighted sampling formula:
// phi = 2*PI*xi1
// cos(theta) = sqrt(xi2), sin(theta) = sqrt(1 - xi2)
// x = sin(theta) * cos(phi)
// y = sin(theta) * sin(phi)
// z = cos(theta)
// Simple hash function for per-pixel randomization
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

vec3 cosineWeightedDirection(int index, float rotationAngle) {
    vec2 xi = aoSamples.samples[index].xy;
    // Add per-pixel rotation to the azimuthal angle
    float phi = 2.0 * 3.14159265359 * xi.x + rotationAngle;
    float cosTheta = sqrt(1.0 - xi.y);
    float sinTheta = sqrt(xi.y);

    return vec3(
        sinTheta * cos(phi),
        sinTheta * sin(phi),
        cosTheta
    );
}

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

    // Build TBN matrix to transform from tangent space to world space
    mat3 TBN = mat3(tangent, bitangent, normal);

    float aoRadius = pushConstants.aoRadius;

    // Per-pixel random rotation to break up banding patterns
    // Using sampleIndex ensures different rotation each frame for same pixel
    float rotationAngle = hash(gl_FragCoord.xy + float(pushConstants.sampleIndex) * 0.1) * 2.0 * 3.14159265359;

    // Single sample this frame using the sample index from push constants
    vec3 sampleDir = TBN * cosineWeightedDirection(pushConstants.sampleIndex, rotationAngle);

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
