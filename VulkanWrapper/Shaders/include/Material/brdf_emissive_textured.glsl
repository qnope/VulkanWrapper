#ifndef BRDF_EMISSIVE_TEXTURED_GLSL
#define BRDF_EMISSIVE_TEXTURED_GLSL

// Emissive BRDF for textured materials with intensity.
// Provides: evaluate_brdf, emissive_light, generate_ray
//
// Requires before #include:
//   - textures[] array declared
//   - #define BRDF_SAMPLER <sampler_name>
//   - vec2 _brdf_uv; file-scope variable (set by caller before each call)
//   - (optional) #define BRDF_HAS_QUERY_LOD for fragment shaders

layout(buffer_reference, scalar, buffer_reference_align = 4)
    readonly buffer EmissiveTexturedMaterialRef {
    uint diffuse_texture_index;
    float intensity;
};

vec3 emissive_light(uint64_t material_address) {
    EmissiveTexturedMaterialRef ref =
        EmissiveTexturedMaterialRef(material_address);
    uint tex_idx = ref.diffuse_texture_index;
    float intensity = ref.intensity;
#ifdef BRDF_HAS_QUERY_LOD
    float lod = textureQueryLod(
        sampler2D(textures[nonuniformEXT(tex_idx)], BRDF_SAMPLER),
        _brdf_uv).x;
#else
    float lod = 1.0;
#endif
    return textureLod(
        sampler2D(textures[nonuniformEXT(tex_idx)], BRDF_SAMPLER),
        _brdf_uv, lod).rgb * intensity;
}

vec3 evaluate_brdf(vec3 normal, uint64_t material_address,
                   vec3 wi, vec3 wo) {
    return vec3(0.0);
}

#ifdef BRDF_HAS_GENERATE_RAY
vec3 generate_ray(uint sample_index, vec2 xi, uint64_t material_address,
                  vec3 normal, vec3 tangeant, vec3 bitangeant) {
    return vec3(0.0);
}
#endif

#endif // BRDF_EMISSIVE_TEXTURED_GLSL
