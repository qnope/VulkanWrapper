#ifndef BRDF_COLORED_GLSL
#define BRDF_COLORED_GLSL

// Lambertian BRDF for colored (untextured) materials.
// Provides: brdf_get_albedo, brdf_get_albedo_alpha, evaluate_brdf,
//           emissive_light, generate_ray
//
// Requires before #include:
//   - ATMO_PI defined (from atmosphere_params.glsl)

layout(buffer_reference, scalar, buffer_reference_align = 4)
    readonly buffer ColoredMaterialRef {
    vec3 color;
};

vec3 brdf_get_albedo(uint64_t material_address) {
    return ColoredMaterialRef(material_address).color;
}

vec4 brdf_get_albedo_alpha(uint64_t material_address) {
    return vec4(ColoredMaterialRef(material_address).color, 1.0);
}

vec3 emissive_light(uint64_t material_address) { return vec3(0.0); }

vec3 evaluate_brdf(vec3 normal, uint64_t material_address,
                   vec3 wi, vec3 wo) {
    return brdf_get_albedo(material_address) / ATMO_PI;
}

#ifdef BRDF_HAS_GENERATE_RAY
vec3 generate_ray(uint sample_index, vec2 xi, uint64_t material_address,
                  vec3 normal, vec3 tangeant, vec3 bitangeant) {
    return sample_hemisphere_cosine(normal, tangeant, bitangeant, xi);
}
#endif

#endif // BRDF_COLORED_GLSL
