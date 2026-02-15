#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#define GEOMETRY_BUFFER_BINDING 10
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

// Bindless texture array for textured materials
layout(set = 1, binding = 0) uniform sampler tex_sampler;
layout(set = 1, binding = 1) uniform texture2D textures[];

layout(location = 0) rayPayloadInEXT vec3 payload;
hitAttributeEXT vec2 bary;

// Buffer reference for textured material data
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer TexturedMaterialRef {
    uint diffuse_texture_index;
};

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

    uint tex_idx =
        TexturedMaterialRef(v.material_address).diffuse_texture_index;
    // lod level can be replaced later by bounce number
    vec3 hit_albedo = textureLod(
        sampler2D(textures[nonuniformEXT(tex_idx)], tex_sampler), v.uv, 1).rgb;

    // Lambertian BRDF at bounce surface:
    // L_bounce = (hit_albedo / PI) * L_sun * solid_angle * NdotL
    vec3 bounce_radiance = (hit_albedo / ATMO_PI)
                         * luminance_from_sun(sky, world_hit_pos,
                                              world_normal, tlas);

    // payload contains the shading point's albedo (from raygen shader).
    // For cosine-weighted sampling, the PI factors cancel:
    // L_out = (1/N) * sum albedo * L_bounce
    vec3 shading_albedo = payload;
    payload = shading_albedo * bounce_radiance;
}
