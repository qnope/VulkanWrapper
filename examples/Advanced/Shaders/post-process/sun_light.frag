#version 460
#extension GL_EXT_ray_query : require

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D samplerColor;
layout (set = 0, binding = 1) uniform sampler2D samplerPosition;
layout (set = 0, binding = 2) uniform sampler2D samplerNormal;
layout (set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;
layout (set = 0, binding = 4) uniform sampler2D samplerAO;

layout (push_constant) uniform PushConstants {
    vec4 sunDirection;
    vec4 sunColor;
} pushConstants;

void main()
{
    vec3 normal = texture(samplerNormal, inUV).rgb;

    vec3 position = texture(samplerPosition, inUV).rgb;
    vec3 albedo = texture(samplerColor, inUV).rgb;

    // Direction TO the sun (negated sun direction which points FROM sun)
    vec3 L = normalize(-pushConstants.sunDirection.xyz);
    float NdotL = max(dot(normal, L), 0.0);

    // Shadow ray tracing using ray query
    float shadow = 1.0;

    // Only trace shadow ray if surface faces the sun
    if (NdotL > 0.0) {
        // Offset along normal to avoid self-intersection
        // Use distance-adaptive bias for numerical stability at large world coordinates
        float bias = max(0.1, length(position) * 0.0005);
        vec3 rayOrigin = position;
        vec3 rayDirection = L;
        float tMin = 1;
        float tMax = 100001.0;

        rayQueryEXT rayQuery;
        rayQueryInitializeEXT(
            rayQuery,
            topLevelAS,
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
            0xFF,
            rayOrigin,
            tMin,
            rayDirection,
            tMax
        );

        // Traverse the acceleration structure
        while (rayQueryProceedEXT(rayQuery)) {
            // For opaque geometry, we don't need to do anything here
        }

        // Check if we hit anything
        if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
            shadow = 0.;
        }
    }

    // Sample ambient occlusion
    float ao = texture(samplerAO, inUV).r;

    // Apply AO to ambient term only
    vec3 diffuse = albedo * (pushConstants.sunColor.rgb * NdotL * shadow + 0.2 * ao);

    outColor = vec4(diffuse, 1.0);
}
