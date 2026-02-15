#version 460
#extension GL_EXT_ray_tracing : require

#include "atmosphere_params.glsl"

layout(push_constant) uniform PushConstants {
    SkyParameters sky;
    uint frame_count;
    uint width;
    uint height;
};

#include "atmosphere_scattering.glsl"
#include "sky_radiance.glsl"

layout(location = 0) rayPayloadInEXT vec3 payload;

void main() {
    // Ray missed all geometry - compute sky radiance in ray direction
    vec3 ray_dir = gl_WorldRayDirectionEXT;
    payload = compute_sky_radiance(sky, ray_dir);
}
