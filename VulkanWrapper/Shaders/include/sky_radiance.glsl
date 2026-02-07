// Sky Radiance - Shared function for computing sky radiance
// Requires atmosphere_params.glsl and atmosphere_scattering.glsl to be
// included first
//
// Usage:
//   #include "atmosphere_params.glsl"
//   layout(push_constant) uniform PushConstants { SkyParameters sky; };
//   #include "atmosphere_scattering.glsl"
//   #include "sky_radiance.glsl"

#ifndef SKY_RADIANCE_GLSL
#define SKY_RADIANCE_GLSL

// Compute single scattering contribution at a sample point along the view ray
// p: atmosphere parameters
// position: sample point in world space
// view_dir: direction of view ray
// sun_dir: direction TO the sun (normalized)
vec3 sky_scattering_at_point(SkyParameters p, vec3 position, vec3 view_dir,
                             vec3 sun_dir) {
    float distance_out_atmosphere = atmo_intersect_ray_sphere_from_inside(
        position, sun_dir, atmo_radius_atmosphere(p));
    vec3 out_atmosphere = position + sun_dir * distance_out_atmosphere;

    vec3 trToSun = atmo_transmittance(p, position, out_atmosphere);
    vec3 rayleigh_diffusion =
        atmo_sigma_s_rayleigh(p, position) * atmo_rayleigh_phase(view_dir, sun_dir);
    vec3 mie_diffusion =
        atmo_sigma_s_mie(p, position) * atmo_mie_phase(view_dir, sun_dir);

    return atmo_sun_radiance(p) * trToSun * atmo_star_solid_angle(p) *
           (rayleigh_diffusion + mie_diffusion);
}

// Integrate single scattering along a view ray
// p: atmosphere parameters
// out_atmosphere: intersection point with atmosphere boundary
// sun_dir: direction TO the sun (normalized)
// Returns: radiance in W/m^2/sr (before luminous efficiency conversion)
vec3 sky_integrate_scattering(SkyParameters p, vec3 out_atmosphere,
                              vec3 sun_dir) {
    vec3 origin = atmo_observer_origin(p);
    vec3 ds = (out_atmosphere - origin) / ATMO_STEPS;
    float step_len = length(ds);

    // Zero-length path (e.g., ray grazing the planet surface) -
    // no atmosphere to integrate through
    if (step_len < 1e-6) return vec3(0.0);

    vec3 direction = ds / step_len;
    vec3 acc = vec3(0.0);

    for (int i = 0; i < ATMO_STEPS; ++i) {
        vec3 s = origin + (float(i) + 0.5) * ds;
        acc += atmo_transmittance(p, origin, s) *
               sky_scattering_at_point(p, s, direction, sun_dir);
    }

    return acc * step_len;
}

// Compute sky radiance for a given view direction
// p: atmosphere parameters
// view_dir: normalized view direction
// Returns: luminance in cd/m^2 (after luminous efficiency conversion)
vec3 compute_sky_radiance(SkyParameters p, vec3 view_dir) {
    // Direction TO the sun (opposite of star_direction which is FROM star)
    vec3 sun_dir = normalize(-atmo_star_direction(p));
    vec3 origin = atmo_observer_origin(p);

    // Find intersection with atmosphere boundary
    float dist_atmo = atmo_intersect_ray_sphere_from_inside(
        origin, view_dir, atmo_radius_atmosphere(p));

    // If the ray hits the planet surface, cap the integration there.
    // Rays going below the horizon should only integrate through the
    // atmosphere above the planet, not through the planet interior.
    float dist_planet = atmo_intersect_ray_sphere_from_outside(
        origin, view_dir, atmo_radius_planet(p));
    float dist_out = (dist_planet > 0.0)
                   ? min(dist_planet, dist_atmo)
                   : dist_atmo;

    vec3 out_pos = origin + view_dir * dist_out;

    // Integrate scattering along the view ray
    vec3 radiance = sky_integrate_scattering(p, out_pos, sun_dir);

    // Convert to luminance
    return radiance * atmo_luminous_efficiency(p);
}

// Compute direct light from sun disk (for sky rendering, not indirect)
// p: atmosphere parameters
// direction: normalized view direction
// out_atmosphere: intersection point with atmosphere boundary
// sun_dir: direction TO the sun (normalized)
// Returns: luminance from sun disk in cd/m^2
vec3 compute_sun_disk_luminance(SkyParameters p, vec3 direction,
                                vec3 out_atmosphere, vec3 sun_dir) {
    float cos_theta = dot(direction, sun_dir);

    // Sun disk angular radius from solid angle
    // solid_angle = 2*PI*(1 - cos(theta))
    // cos(theta) = 1 - solid_angle/(2*PI)
    // theta = acos(1 - solid_angle/(2*PI))
    float radius_size = acos(1.0 - atmo_star_solid_angle(p) / (2.0 * ATMO_PI));
    float angle = acos(cos_theta);
    float disk = 1.0 - smoothstep(radius_size * 0.95, radius_size * 1.05, angle);

    return disk * atmo_sun_radiance(p) *
           atmo_transmittance(p, atmo_observer_origin(p), out_atmosphere) *
           atmo_luminous_efficiency(p);
}

#endif // SKY_RADIANCE_GLSL
