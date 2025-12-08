#version 460
#extension GL_EXT_ray_query : require

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D samplerDepth;
layout (set = 0, binding = 1) uniform sampler2D samplerNormal;
layout (set = 0, binding = 2) uniform accelerationStructureEXT topLevelAS;

layout (push_constant) uniform PushConstants {
    mat4 inverseViewProj;
    float aoScale;
    float aoIntensity;
    int aoSamples;
    uint frameIndex;
} pc;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const float RAY_MAX = 1e30;

uvec3 pcg3d(uvec3 v) {
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v;
}

float uintToFloat(uint x) {
    return float(x) * (1.0 / 4294967296.0);
}

vec2 random2D(uvec2 pixel, uint sampleIndex, uint frameIndex) {
    uvec3 seed = uvec3(pixel, sampleIndex + frameIndex * 256u);
    uvec3 hash = pcg3d(seed);
    return vec2(uintToFloat(hash.x), uintToFloat(hash.y));
}

vec3 reconstructWorldPosition(vec2 uv, float depth) {
    vec2 ndc = vec2(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0);
    vec4 clipPos = vec4(ndc, depth, 1.0);
    vec4 worldPos = pc.inverseViewProj * clipPos;
    return worldPos.xyz / worldPos.w;
}

void buildOrthonormalBasis(vec3 n, out vec3 tangent, out vec3 bitangent) {
    if (n.z < -0.9999999) {
        tangent = vec3(0.0, -1.0, 0.0);
        bitangent = vec3(-1.0, 0.0, 0.0);
    } else {
        float a = 1.0 / (1.0 + n.z);
        float b = -n.x * n.y * a;
        tangent = vec3(1.0 - n.x * n.x * a, b, -n.x);
        bitangent = vec3(b, 1.0 - n.y * n.y * a, -n.y);
    }
}

// Uniform hemisphere sampling (not cosine-weighted)
// This gives equal weight to all directions
vec3 sampleUniformHemisphere(vec2 u) {
    float phi = TWO_PI * u.x;
    float cosTheta = u.y;  // uniform in [0,1]
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

// Visibility function: how much of this ray direction is "visible" (unoccluded)
// Returns 1 if ray escapes to infinity, decreases based on how close the hit is
float computeVisibility(float hitDistance, float scale) {
    // visibility = distance / (distance + scale)
    // At distance 0: visibility = 0 (fully occluded)
    // At distance = scale: visibility = 0.5
    // At distance = infinity: visibility = 1 (fully visible)
    return hitDistance / (hitDistance + scale);
}

void main() {
    float depth = texture(samplerDepth, inUV).r;

    if (depth >= 1.0) {
        outColor = vec4(0.4, 0.5, 1.0, 1.0); // Blue sky
        return;
    }

    vec3 normal = normalize(texture(samplerNormal, inUV).rgb);
    vec3 position = reconstructWorldPosition(inUV, depth);

    vec3 tangent, bitangent;
    buildOrthonormalBasis(normal, tangent, bitangent);

    uvec2 pixelCoord = uvec2(gl_FragCoord.xy);

    // Offset of 1 along normal
    vec3 rayOrigin = position + normal * 1.0;

    float totalVisibility = 0.0;

    for (int i = 0; i < pc.aoSamples; i++) {
        vec2 u = random2D(pixelCoord, uint(i), pc.frameIndex);
        float stratum = float(i) / float(pc.aoSamples);
        u.x = fract(u.x + stratum);

        vec3 localDir = sampleUniformHemisphere(u);
        vec3 rayDir = tangent * localDir.x + bitangent * localDir.y + normal * localDir.z;

        rayQueryEXT rayQuery;
        rayQueryInitializeEXT(
            rayQuery,
            topLevelAS,
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
            0xFF,
            rayOrigin,
            0.0,
            rayDir,
            RAY_MAX
        );

        while (rayQueryProceedEXT(rayQuery)) {}

        if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
            float hitT = rayQueryGetIntersectionTEXT(rayQuery, true);
            // Visibility based on hit distance
            totalVisibility += computeVisibility(hitT, pc.aoScale);
        } else {
            // Ray escaped - full visibility
            totalVisibility += 1.0;
        }
    }

    // Average visibility across all samples
    float avgVisibility = totalVisibility / float(pc.aoSamples);

    // Apply intensity as a power curve to control contrast
    float ao = pow(avgVisibility, pc.aoIntensity);

    outColor = vec4(ao, ao, ao, 1.0);
}
