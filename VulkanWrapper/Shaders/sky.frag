#version 450

layout(location = 0) in vec2 in_position;

#include "atmosphere_params.glsl"

// SkyParametersGPU + inverseViewProj as push constants
layout(push_constant) uniform PushConstants {
    SkyParameters sky;
    mat4 inverseViewProj;
} pc;

#include "atmosphere_scattering.glsl"
#include "sky_radiance.glsl"

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

void main() {
    SkyParameters p = pc.sky;

    vec3 view_direction = get_view_direction();
    // Direction TO the sun (opposite of star_direction which is FROM star)
    vec3 sun_dir = normalize(-atmo_star_direction(p));

    vec3 origin = atmo_observer_origin(p);
    float distance_out = atmo_intersect_ray_sphere_from_inside(
        origin, view_direction, atmo_radius_atmosphere(p));
    vec3 view_out = origin + view_direction * distance_out;

    // Use shared sky radiance function
    vec3 luminance = compute_sky_radiance(p, view_direction);

    // Add direct sun disk contribution
    luminance += compute_sun_disk_luminance(p, view_direction, view_out, sun_dir);

    out_color = vec4(luminance, 1.0);
}
