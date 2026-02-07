// Atmosphere Scattering - Ray intersection and scattering calculations
// Requires atmosphere_params.glsl to be included first

#ifndef ATMOSPHERE_SCATTERING_GLSL
#define ATMOSPHERE_SCATTERING_GLSL

// Parameter extraction functions

// Scale heights
float atmo_height_rayleigh(SkyParameters p) { return p.rayleigh_and_height_r.w; }
float atmo_height_mie(SkyParameters p) { return p.mie_and_height_m.w; }
float atmo_height_ozone(SkyParameters p) { return p.ozone_and_height_o.w; }

// Scattering/absorption coefficients
vec3 atmo_rayleigh_coef(SkyParameters p) { return p.rayleigh_and_height_r.xyz; }
vec3 atmo_mie_coef(SkyParameters p) { return p.mie_and_height_m.xyz; }
vec3 atmo_ozone_coef(SkyParameters p) { return p.ozone_and_height_o.xyz; }

// Planet geometry
float atmo_radius_planet(SkyParameters p) { return p.radii_and_efficiency.x; }
float atmo_radius_atmosphere(SkyParameters p) { return p.radii_and_efficiency.y; }

// Star/sun properties
float atmo_luminous_efficiency(SkyParameters p) { return p.radii_and_efficiency.z; }
float atmo_star_constant(SkyParameters p) { return p.star_direction_and_constant.w; }
float atmo_star_solid_angle(SkyParameters p) { return p.star_color_and_solid_angle.w; }
vec3 atmo_star_direction(SkyParameters p) { return p.star_direction_and_constant.xyz; }
vec3 atmo_star_color(SkyParameters p) { return p.star_color_and_solid_angle.xyz; }

// Derived values
float atmo_zenith_height(SkyParameters p) {
    return atmo_radius_atmosphere(p) - atmo_radius_planet(p);
}

vec3 atmo_observer_origin(SkyParameters p) {
    return vec3(0, atmo_radius_planet(p) + 1, 0);
}

// Compute sun disk radiance from solar constant and solid angle
vec3 atmo_sun_radiance(SkyParameters p) {
    return vec3(atmo_star_constant(p) / atmo_star_solid_angle(p));
}

// Ray-sphere intersection for rays starting inside the sphere
// Returns the distance to the far intersection point
float atmo_intersect_ray_sphere_from_inside(vec3 rayOrigin, vec3 rayDir, float radius) {
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - radius * radius;
    float discriminant = b * b - c;
    return -b + sqrt(discriminant);
}

// Ray-sphere intersection for rays starting outside the sphere
// Returns the distance to the near intersection point, or -1.0 if no hit
float atmo_intersect_ray_sphere_from_outside(vec3 rayOrigin, vec3 rayDir, float radius) {
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - radius * radius;
    float discriminant = b * b - c;
    if (discriminant < 0.0) return -1.0;
    float t = -b - sqrt(discriminant);
    return (t > 0.0) ? t : -1.0;
}

// Altitude above planet surface, clamped to 0 to prevent density blow-up
// for sample points below the surface (e.g., when integrating through the
// planet along sun-direction transmittance paths).
float atmo_altitude(SkyParameters p, vec3 position) {
    return max(0.0, length(position) - atmo_radius_planet(p));
}

// Rayleigh scattering coefficient at given position
vec3 atmo_sigma_s_rayleigh(SkyParameters p, vec3 position) {
    float h = atmo_altitude(p, position);
    return atmo_rayleigh_coef(p) * exp(-h / atmo_height_rayleigh(p));
}

// Mie scattering coefficient at given position
vec3 atmo_sigma_s_mie(SkyParameters p, vec3 position) {
    float h = atmo_altitude(p, position);
    return atmo_mie_coef(p) * exp(-h / atmo_height_mie(p));
}

// Ozone absorption coefficient at given position
vec3 atmo_sigma_a_ozone(SkyParameters p, vec3 position) {
    float h = atmo_altitude(p, position);
    return atmo_ozone_coef(p) * exp(-h / atmo_height_ozone(p));
}

// Total extinction coefficient at given position
vec3 atmo_sigma_t(SkyParameters p, vec3 position) {
    return atmo_sigma_s_rayleigh(p, position) +
           1.11 * atmo_sigma_s_mie(p, position) +
           atmo_sigma_a_ozone(p, position);
}

// Integrate optical depth along a path
vec3 atmo_integrate_optical_depth(SkyParameters p, vec3 from, vec3 to) {
    vec3 ds = (to - from) / float(ATMO_STEPS);
    vec3 accumulation = vec3(0.0);

    for (int i = 0; i < ATMO_STEPS; ++i) {
        vec3 s = from + (float(i) + 0.5) * ds;
        accumulation += atmo_sigma_t(p, s);
    }

    return accumulation * length(ds);
}

// Compute transmittance between two points
vec3 atmo_transmittance(SkyParameters p, vec3 from, vec3 to) {
    vec3 integral = atmo_integrate_optical_depth(p, from, to);
    return exp(-integral);
}

// Compute transmittance from a point to outer space along a direction
vec3 atmo_transmittance_to_space(SkyParameters p, vec3 position, vec3 direction) {
    float distance_out = atmo_intersect_ray_sphere_from_inside(
        position, direction, atmo_radius_atmosphere(p));
    vec3 out_atmosphere = position + direction * distance_out;
    return atmo_transmittance(p, position, out_atmosphere);
}

// Rayleigh phase function
float atmo_rayleigh_phase(vec3 view_dir, vec3 sun_dir) {
    float mu = dot(view_dir, sun_dir);
    return (3.0 / (16.0 * ATMO_PI)) * (1.0 + mu * mu);
}

// Mie phase function (Henyey-Greenstein with g = 0.76)
float atmo_mie_phase(vec3 view_dir, vec3 sun_dir) {
    float mu = dot(view_dir, sun_dir);
    float g = 0.76;
    float denom = 1.0 + g * g - 2.0 * g * mu;
    return (1.0 - g * g) / (4.0 * ATMO_PI * pow(denom, 1.5));
}

#endif // ATMOSPHERE_SCATTERING_GLSL
