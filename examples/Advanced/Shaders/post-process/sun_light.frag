#version 460
#extension GL_EXT_ray_query : require

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D samplerColor;
layout (set = 0, binding = 1) uniform sampler2D samplerPosition;
layout (set = 0, binding = 2) uniform sampler2D samplerNormal;
layout (set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;

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
        // Small offset along normal to avoid self-intersection
        vec3 rayOrigin = position + normal * 0.001;
        vec3 rayDirection = L;
        float tMin = 0.001;
        float tMax = 1000.0;

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
            shadow = 0.0;
        }
    }

    vec3 diffuse = albedo * pushConstants.sunColor.rgb * NdotL * shadow;

    outColor = vec4(diffuse, 1.0);
}
