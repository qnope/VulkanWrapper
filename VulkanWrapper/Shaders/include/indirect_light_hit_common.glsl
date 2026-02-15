// Indirect Light Hit Common - Shared code for all indirect light closest hit
// shaders. Provides vertex interpolation, world-space transforms, and
// Lambertian BRDF computation.
//
// Usage: Include this file after #version and any material-specific extensions.
// Then define your material-specific bindings, buffer references, and main():
//
//   #version 460
//   #include "indirect_light_hit_common.glsl"
//
//   // material-specific bindings and buffer references here...
//
//   void main() {
//       HitInfo hit = compute_hit_info();
//       if (!hit.valid) { payload = vec3(0.0); return; }
//
//       vec3 albedo = /* read from your material */;
//       payload = compute_bounce_radiance(albedo, hit);
//   }

#ifndef INDIRECT_LIGHT_HIT_COMMON_GLSL
#define INDIRECT_LIGHT_HIT_COMMON_GLSL

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

#define GEOMETRY_BUFFER_BINDING 7
#include "geometry_access.glsl"

#include "atmosphere_params.glsl"

layout(push_constant) uniform PushConstants {
    SkyParameters sky;
    uint frame_count;
    uint width;
    uint height;
};

#include "atmosphere_scattering.glsl"
#include "sun_lighting_computation.glsl"

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT vec3 payload;
hitAttributeEXT vec2 bary;

// Result of vertex interpolation and world-space transform
struct HitInfo {
    VertexData vertex;
    vec3 world_hit_pos;
    vec3 world_normal;
    bool valid;
};

// Interpolate vertex attributes and compute world-space hit info.
// Sets valid=false for degenerate normals.
HitInfo compute_hit_info() {
    HitInfo info;
    info.vertex = interpolate_vertex(
        gl_InstanceCustomIndexEXT, gl_PrimitiveID, bary);

    info.world_hit_pos = gl_WorldRayOriginEXT
                       + gl_WorldRayDirectionEXT * gl_HitTEXT;

    vec3 raw_normal = mat3(gl_ObjectToWorldEXT) * info.vertex.normal;
    float normal_len = length(raw_normal);
    info.valid = normal_len >= 1e-6;
    info.world_normal = info.valid ? raw_normal / normal_len : vec3(0.0);

    return info;
}

// Lambertian BRDF at bounce surface:
// L_bounce = (albedo / PI) * L_sun * solid_angle * NdotL
vec3 compute_bounce_radiance(vec3 albedo, HitInfo hit) {
    return (albedo / ATMO_PI)
         * luminance_from_sun(sky, hit.world_hit_pos,
                              hit.world_normal, tlas);
}

#endif // INDIRECT_LIGHT_HIT_COMMON_GLSL
