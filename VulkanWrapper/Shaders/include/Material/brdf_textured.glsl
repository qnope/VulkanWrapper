#ifndef BRDF_TEXTURED_GLSL
#define BRDF_TEXTURED_GLSL

// Lambertian BRDF for textured materials.
// Provides: brdf_get_albedo, brdf_get_albedo_alpha, evaluate_brdf
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

vec4 brdf_get_albedo_alpha(uint64_t material_address) {
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
        _brdf_uv, lod);
}

vec3 brdf_get_albedo(uint64_t material_address) {
    return brdf_get_albedo_alpha(material_address).rgb;
}

vec3 evaluate_brdf(vec3 normal, uint64_t material_address,
                   vec3 wi, vec3 wo) {
    return brdf_get_albedo(material_address) / ATMO_PI;
}

#endif // BRDF_TEXTURED_GLSL
