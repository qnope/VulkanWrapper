#version 450

layout(location = 0) in vec2 in_position;

#include "atmosphere_params.glsl"

// SkyParametersGPU + inverseViewProj as push constants
layout(push_constant) uniform PushConstants {
    SkyParameters sky;
    mat4 inverseViewProj;
} pc;

#include "atmosphere_scattering.glsl"

layout(location = 0) out vec4 out_color;

vec3 get_world_pos(vec4 clipPosition)
{
    vec4 world = pc.inverseViewProj * clipPosition;
    world /= world.w;
    return world.xyz;
}

vec3 get_view_direction() {
    // Convert UV coordinates [0,1] to clip space [-1,1]
    vec2 clip_pos = in_position * 2.0 - 1.0;
    vec3 near = get_world_pos(vec4(clip_pos, 0.0, 1.0));
    vec3 far = get_world_pos(vec4(clip_pos, 1.0, 1.0));
    return normalize(far - near);
}

vec3 j(SkyParameters p, vec3 position, vec3 view_dir, vec3 sun_dir) {
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

vec3 compute_luminance(SkyParameters p, vec3 out_atmosphere, vec3 sun_dir) {
    vec3 origin = atmo_observer_origin(p);
    vec3 ds = (out_atmosphere - origin) / ATMO_STEPS;
    vec3 direction = normalize(ds);
    vec3 acc = vec3(0.0);

    for (int i = 0; i < ATMO_STEPS; ++i) {
        vec3 s = origin + (float(i) + 0.5) * ds;
        acc += atmo_transmittance(p, origin, s) * j(p, s, direction, sun_dir);
    }

    return acc * length(ds);
}

vec3 direct_light_from_sun(SkyParameters p, vec3 direction, vec3 out_atmosphere, vec3 sun_dir) {
    float cos_theta = dot(direction, sun_dir);

    // Sun disk angular radius from solid angle
    // solid_angle = 2*PI*(1 - cos(theta))
    // cos(theta) = 1 - solid_angle/(2*PI)
    // theta = acos(1 - solid_angle/(2*PI))
    float radius_size = acos(1.0 - atmo_star_solid_angle(p) / (2.0 * ATMO_PI));
    float angle = acos(cos_theta);
    float disk = 1.0 - smoothstep(radius_size * 0.95, radius_size * 1.05, angle);

    return disk * atmo_sun_radiance(p) *
           atmo_transmittance(p, atmo_observer_origin(p), out_atmosphere);
}

void main() {
    SkyParameters p = pc.sky;

    vec3 view_direction = get_view_direction();
    // Direction TO the sun (opposite of star_direction which is FROM star)
    vec3 sun_dir = normalize(-atmo_star_direction(p));

    vec3 origin = atmo_observer_origin(p);
    float distance_out = atmo_intersect_ray_sphere_from_inside(
        origin, view_direction, atmo_radius_atmosphere(p));
    vec3 view_out = origin + view_direction * distance_out;

    vec3 radiance = compute_luminance(p, view_out, sun_dir);
    radiance += direct_light_from_sun(p, view_direction, view_out, sun_dir);

    // Convert from W/m^2/sr to cd/m^2 using luminous efficiency
    vec3 luminance = radiance * atmo_luminous_efficiency(p);

    out_color = vec4(luminance, 1.0);
}
