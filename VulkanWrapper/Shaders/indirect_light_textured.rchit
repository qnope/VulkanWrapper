#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "indirect_light_hit_common.glsl"

// Bindless texture array for textured materials
layout(set = 1, binding = 0) uniform sampler tex_sampler;
layout(set = 1, binding = 1) uniform texture2D textures[];

// Buffer reference for textured material data
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer TexturedMaterialRef {
    uint diffuse_texture_index;
};

void main() {
    HitInfo hit = compute_hit_info();
    if (!hit.valid) {
        payload = vec3(0.0);
        return;
    }

    uint tex_idx =
        TexturedMaterialRef(hit.vertex.material_address).diffuse_texture_index;
    // lod level can be replaced later by bounce number
    vec3 albedo = textureLod(
        sampler2D(textures[nonuniformEXT(tex_idx)], tex_sampler),
        hit.vertex.uv, 1).rgb;

    payload = compute_bounce_radiance(albedo, hit);
}
