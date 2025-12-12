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
const int MAX_SAMPLES = 256;

// UBO containing random values for cosine-weighted sampling
layout (set = 0, binding = 5) uniform AOSamples {
    // xi1 is used for azimuthal angle (phi = 2*PI*xi1)
    // xi2 is used for polar angle via cosine-weighted distribution (cos(theta) = sqrt(xi2))
    vec4 samples[MAX_SAMPLES]; // xy = (xi1, xi2) for each sample
} aoSamples;

layout (push_constant) uniform PushConstants {
    float aoRadius;
    int numSamples;
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

    float occlusion = 0.0;
    float aoRadius = pushConstants.aoRadius;

    // Per-pixel random rotation to break up banding patterns
    float rotationAngle = hash(gl_FragCoord.xy) * 2.0 * 3.14159265359;

    int numSamples = min(pushConstants.numSamples, MAX_SAMPLES);
    for (int i = 0; i < numSamples; ++i) {
        // Get cosine-weighted direction in tangent space and transform to world space
        vec3 sampleDir = TBN * cosineWeightedDirection(i, rotationAngle);

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
            aoRadius  // Effectively infinite
        );

        // Traverse the acceleration structure
        while (rayQueryProceedEXT(rayQuery)) {
            // For opaque geometry, no processing needed
        }

        // Check if we hit anything
        if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
            occlusion += 1.0;
        }
    }

    // Average the occlusion
    occlusion /= float(numSamples);

    // Convert occlusion to visibility (1 = fully visible, 0 = fully occluded)
    float visibility = 1.0 - clamp(occlusion, 0.0, 1.0);

    // Output greyscale RGBA
    outColor = vec4(visibility, visibility, visibility, 1.0);

    // DEBUG: visualize normals (comment out for normal AO)
    // outColor = vec4(normal * 0.5 + 0.5, 1.0);
}
