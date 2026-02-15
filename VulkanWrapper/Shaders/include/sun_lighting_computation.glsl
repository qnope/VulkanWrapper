// Sun Lighting Computation - Shared sun lighting with shadow rays
// Requires atmosphere_params.glsl and atmosphere_scattering.glsl to be
// included first, and GL_EXT_ray_query to be enabled.

#ifndef SUN_LIGHTING_COMPUTATION_GLSL
#define SUN_LIGHTING_COMPUTATION_GLSL

// Compute the sun's physical radiance arriving at a surface point,
// accounting for atmospheric transmittance, shadow occlusion, and the
// cosine term.  Returns vec3(0) when the surface faces away from the
// sun or is in shadow.  Does NOT include albedo/BRDF.
//
// Result units: W/m^2  (radiance * solid_angle * NdotL * shadow)
vec3 radiance_from_sun(SkyParameters sky, vec3 position, vec3 normal,
                       accelerationStructureEXT tlas) {
    // Direction TO the sun (negated star direction which points FROM sun)
    vec3 L = normalize(-atmo_star_direction(sky));
    float NdotL = max(dot(normal, L), 0.0);

    if (NdotL <= 0.0)
        return vec3(0.0);

    // Shadow ray using ray query
    float bias = max(0.5, length(position) * 0.0005);
    vec3 ray_origin = position + normal * bias;

    rayQueryEXT shadow_query;
    rayQueryInitializeEXT(
        shadow_query, tlas,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
        0xFF,
        ray_origin, 0.5, L, 100000.0);

    while (rayQueryProceedEXT(shadow_query)) {}

    if (rayQueryGetIntersectionTypeEXT(shadow_query, true) !=
        gl_RayQueryCommittedIntersectionNoneEXT) {
        return vec3(0.0);
    }

    // Atmospheric transmittance along sun direction
    vec3 atmo_trans = atmo_transmittance_to_space(
        sky, atmo_observer_origin(sky), L);

    // Sun radiance at ground level (W/m^2/sr)
    vec3 L_sun = atmo_sun_radiance(sky) * atmo_trans
               * atmo_star_color(sky);

    float solid_angle = atmo_star_solid_angle(sky);

    return L_sun * solid_angle * NdotL;
}

// Compute sun luminance arriving at a surface point.
// Equivalent to radiance_from_sun() scaled by luminous efficiency.
//
// Result units: cd/m^2  (luminance * solid_angle * NdotL * shadow)
vec3 luminance_from_sun(SkyParameters sky, vec3 position, vec3 normal,
                        accelerationStructureEXT tlas) {
    return radiance_from_sun(sky, position, normal, tlas)
         * atmo_luminous_efficiency(sky);
}

#endif // SUN_LIGHTING_COMPUTATION_GLSL
