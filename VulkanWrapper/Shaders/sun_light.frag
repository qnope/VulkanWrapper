#version 460
#extension GL_EXT_ray_query : require

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D samplerColor;
layout (set = 0, binding = 1) uniform sampler2D samplerPosition;
layout (set = 0, binding = 2) uniform sampler2D samplerNormal;
layout (set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;

#include "atmosphere_params.glsl"

// SkyParametersGPU - matches C++ struct layout exactly
layout (push_constant) uniform PushConstants {
    SkyParameters sky;
};

#include "atmosphere_scattering.glsl"

void main()
{
    vec3 normal = texture(samplerNormal, inUV).rgb;
    vec3 position = texture(samplerPosition, inUV).rgb;
    vec3 albedo = texture(samplerColor, inUV).rgb;

    // Direction TO the sun (negated star direction which points FROM sun)
    vec3 L = normalize(-atmo_star_direction(sky));
    float NdotL = max(dot(normal, L), 0.0);

    // Shadow ray tracing using ray query
    float shadow = 1.0;

    // Only trace shadow ray if surface faces the sun
    if (NdotL > 0.0) {
        // Offset along normal to avoid self-intersection
        // Use distance-adaptive bias for numerical stability at large world coordinates
        float bias = max(0.5, length(position) * 0.0005);
        vec3 rayOrigin = position + normal * bias;
        vec3 rayDirection = L;
        float tMin = 0.5;
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
        if (rayQueryGetIntersectionTypeEXT(rayQuery, true) !=
            gl_RayQueryCommittedIntersectionNoneEXT) {
            shadow = 0.;
        }
    }

    // Compute atmospheric transmittance using physical Rayleigh, Mie and ozone
    vec3 atmo_trans = atmo_transmittance_to_space(sky, atmo_observer_origin(sky), L);

    // Compute sun radiance at ground level (W/m^2/sr)
    // L_sun = (solar_constant / solid_angle) * transmittance * star_color
    vec3 L_sun = atmo_sun_radiance(sky) * atmo_trans * atmo_star_color(sky);

    // Convert to luminance units (cd/m^2/sr) using luminous efficiency
    L_sun *= atmo_luminous_efficiency(sky);

    // Physically-based lighting using radiance formulation:
    // L_out = BRDF * L_sun * solid_angle * cos(theta)
    // For Lambertian: BRDF = albedo / PI
    float solid_angle = atmo_star_solid_angle(sky);
    vec3 diffuse = (albedo / ATMO_PI) * L_sun * solid_angle * NdotL * shadow;

    // Output final luminance (cd/m^2)
    // Note: Ambient/indirect lighting with AO is now handled by SkyLightPass
    outColor = vec4(diffuse, 1.0);
}
