#ifndef INDIRECT_LIGHT_BASE_GLSL
#define INDIRECT_LIGHT_BASE_GLSL

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : require
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

// Bindless texture descriptor set
layout(set = 1, binding = 0) uniform sampler tex_sampler;
layout(set = 1, binding = 1) uniform texture2D textures[];

#define BRDF_SAMPLER tex_sampler
vec2 _brdf_uv;

// Forward declarations - implemented by material BRDF include
vec3 evaluate_brdf(vec3 normal, uint64_t material_address,
                   vec3 wi, vec3 wo);
vec3 emissive_light(uint64_t material_address);

layout(location = 0) rayPayloadInEXT vec3 payload;
hitAttributeEXT vec2 bary;

void main() {
    // Interpolate vertex attributes at the hit point
    VertexData v = interpolate_vertex(
        gl_InstanceCustomIndexEXT, gl_PrimitiveID, bary);

    // World-space hit position from ray parameters
    vec3 world_hit_pos = gl_WorldRayOriginEXT
                       + gl_WorldRayDirectionEXT * gl_HitTEXT;

    // Transform normal to world space (guard against degenerate normals)
    vec3 raw_normal = mat3(gl_ObjectToWorldEXT) * v.normal;
    float normal_len = length(raw_normal);
    if (normal_len < 1e-6) {
        payload = vec3(0.0);
        return;
    }
    vec3 world_normal = raw_normal / normal_len;

    _brdf_uv = v.uv;

    // wi = incoming light direction (towards the camera/ray origin)
    // wo = outgoing light direction (towards the sun)
    vec3 wi = -gl_WorldRayDirectionEXT;
    vec3 wo = normalize(-atmo_star_direction(sky));

    vec3 brdf = evaluate_brdf(world_normal, v.material_address,
                              wi, wo);

    payload = emissive_light(v.material_address)
            + brdf * luminance_from_sun(sky, world_hit_pos,
                                        world_normal, tlas);
}

#endif // INDIRECT_LIGHT_BASE_GLSL
