#version 460

#include "indirect_light_hit_common.glsl"

// Buffer reference for colored material data
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer ColoredMaterialRef {
    vec3 color;
};

void main() {
    HitInfo hit = compute_hit_info();
    if (!hit.valid) {
        payload = vec3(0.0);
        return;
    }

    vec3 albedo = ColoredMaterialRef(hit.vertex.material_address).color;
    payload = compute_bounce_radiance(albedo, hit);
}
