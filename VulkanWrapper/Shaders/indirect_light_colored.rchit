#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
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

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT vec3 payload;
hitAttributeEXT vec2 bary;

// Buffer reference for colored material data
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer ColoredMaterialRef {
    vec3 color;
};

// Extract vec3 from vec4 push constant without using .xyz swizzle
// to work around a llvmpipe code generation bug
vec3 extract_xyz(vec4 v) {
    return vec3(v.x, v.y, v.z);
}

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

    // Read albedo from colored material buffer reference
    vec3 hit_albedo = ColoredMaterialRef(v.material_address).color;

    // Direction TO the sun (extract components individually)
    vec3 sun_from = sky.star_direction_and_constant.xyz;
    vec3 L = normalize(-sun_from);
    float NdotL = max(dot(world_normal, L), 0.0);

    if (NdotL <= 0.0) {
        // Surface faces away from sun -- no direct sun contribution
        payload = vec3(0.0);
        return;
    }

    // Shadow test using ray query (avoids increasing recursion depth)
    float shadow = 1.0;
    {
        float bias = max(0.5, length(world_hit_pos) * 0.0005);
        vec3 shadow_origin = world_hit_pos + world_normal * bias;

        rayQueryEXT shadow_query;
        rayQueryInitializeEXT(
            shadow_query, tlas,
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
            0xFF,
            shadow_origin, 0.5, L, 100000.0
        );

        while (rayQueryProceedEXT(shadow_query)) {}

        if (rayQueryGetIntersectionTypeEXT(shadow_query, true) !=
            gl_RayQueryCommittedIntersectionNoneEXT) {
            shadow = 0.0;
        }
    }

    if (shadow <= 0.0) {
        payload = vec3(0.0);
        return;
    }

    // Sun radiance at ground level
    vec3 atmo_trans = atmo_transmittance_to_space(
        sky, atmo_observer_origin(sky), L);
    vec3 star_color = sky.star_color_and_solid_angle.xyz;
    vec3 L_sun = atmo_sun_radiance(sky) * atmo_trans * star_color;
    L_sun *= atmo_luminous_efficiency(sky);

    // Lambertian BRDF at bounce surface:
    // L_bounce = (hit_albedo / PI) * L_sun * solid_angle * NdotL
    float solid_angle = atmo_star_solid_angle(sky);
    vec3 bounce_radiance = (hit_albedo / ATMO_PI) * L_sun * solid_angle
                         * NdotL * shadow;

    // payload contains the shading point's albedo (from raygen shader).
    // For cosine-weighted sampling, the PI factors cancel:
    // L_out = (1/N) * sum albedo * L_bounce
    vec3 shading_albedo = payload;
    payload = shading_albedo * bounce_radiance;
}
