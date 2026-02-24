#ifndef BRDF_TEXTURED_GLSL
#define BRDF_TEXTURED_GLSL

// Lambertian BRDF for textured materials.
// Provides: evaluate_brdf, emissive_light, generate_ray
//
// Requires before #include:
//   - ATMO_PI defined (from atmosphere_params.glsl)
//   - textures[] array declared
//   - #define BRDF_SAMPLER <sampler_name>
//   - vec2 _brdf_uv; file-scope variable (set by caller before each call)
//   - (optional) #define BRDF_HAS_QUERY_LOD for fragment shaders

layout(buffer_reference, scalar, buffer_reference_align = 4)
    readonly buffer TexturedMaterialRef {
    uint diffuse_texture_index;
};

vec3 emissive_light(uint64_t material_address) { return vec3(0.0); }

vec3 evaluate_brdf(vec3 normal, uint64_t material_address,
                   vec3 wi, vec3 wo) {
    uint tex_idx =
        TexturedMaterialRef(material_address).diffuse_texture_index;
#ifdef BRDF_HAS_QUERY_LOD
    float lod = textureQueryLod(
        sampler2D(textures[nonuniformEXT(tex_idx)], BRDF_SAMPLER),
        _brdf_uv).x;
#else
    float lod = 1.0;
#endif
    return textureLod(
        sampler2D(textures[nonuniformEXT(tex_idx)], BRDF_SAMPLER),
        _brdf_uv, lod).rgb / ATMO_PI;
}

#ifdef BRDF_HAS_GENERATE_RAY
vec3 generate_ray(uint sample_index, vec2 xi, uint64_t material_address,
                  vec3 normal, vec3 tangeant, vec3 bitangeant) {
    return sample_hemisphere_cosine(normal, tangeant, bitangeant, xi);
}
#endif

#endif // BRDF_TEXTURED_GLSL
