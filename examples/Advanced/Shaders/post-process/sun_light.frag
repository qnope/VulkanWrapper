#version 460
#extension GL_EXT_ray_query : require

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D samplerColor;
layout (set = 0, binding = 1) uniform sampler2D samplerPosition;
layout (set = 0, binding = 2) uniform sampler2D samplerNormal;
layout (set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;
layout (set = 0, binding = 4) uniform sampler2D samplerAO;

// SkyParametersGPU - matches C++ struct layout exactly
layout (push_constant) uniform PushConstants {
    vec4 star_direction_and_constant;   // xyz = direction FROM star, w = constant
    vec4 star_color_and_solid_angle;    // xyz = color, w = solid_angle
    vec4 rayleigh_and_height_r;         // xyz = rayleigh_coef, w = height_rayleigh
    vec4 mie_and_height_m;              // xyz = mie_coef, w = height_mie
    vec4 ozone_and_height_o;            // xyz = ozone_coef, w = height_ozone
    vec4 radii_and_efficiency;          // x = radius_planet, y = radius_atmosphere, z = luminous_efficiency
} sky;

const float PI = 3.14159265359;
const int STEPS = 8;

// Extract parameters from push constants
float Hr() { return sky.rayleigh_and_height_r.w; }
float Hm() { return sky.mie_and_height_m.w; }
float Ho() { return sky.ozone_and_height_o.w; }
vec3 rayleigh() { return sky.rayleigh_and_height_r.xyz; }
vec3 mie() { return sky.mie_and_height_m.xyz; }
vec3 ozone() { return sky.ozone_and_height_o.xyz; }
float radiusEarth() { return sky.radii_and_efficiency.x; }
float radiusAtmo() { return sky.radii_and_efficiency.y; }
float luminousEfficiency() { return sky.radii_and_efficiency.z; }
float starConstant() { return sky.star_direction_and_constant.w; }
float starSolidAngle() { return sky.star_color_and_solid_angle.w; }

// Compute sun radiance from solar constant and solid angle (W/m²/sr)
vec3 sunRadiance() {
    return vec3(starConstant() / starSolidAngle());
}

// Observer position: 1 meter above Earth surface
vec3 origin_view() { return vec3(0, radiusEarth() + 1, 0); }

float intersectRaySphereFromInside(const vec3 rayOrigin,
                                   const vec3 rayDir, float radius) {
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - radius * radius;
    float discriminant = b * b - c;
    float t = -b + sqrt(discriminant);
    return t;
}

vec3 sigma_s_rayleigh(vec3 position) {
    const float h = length(position) - radiusEarth();
    return rayleigh() * exp(-h / Hr());
}

vec3 sigma_s_mie(vec3 position) {
    const float h = length(position) - radiusEarth();
    return mie() * exp(-h / Hm());
}

vec3 sigma_a_ozone(vec3 position) {
    const float h = length(position) - radiusEarth();
    return ozone() * exp(-h / Ho());
}

vec3 sigma_t(vec3 position) {
    return sigma_s_rayleigh(position) +
           1.11 * sigma_s_mie(position) +
           sigma_a_ozone(position);
}

vec3 integrate_sigma_t(vec3 from, vec3 to) {
    const vec3 ds = (to - from) / float(STEPS);
    vec3 accumulation = vec3(0, 0, 0);

    for (int i = 0; i < STEPS; ++i) {
        const vec3 s = from + (i + 0.5) * ds;
        accumulation += sigma_t(s);
    }

    return accumulation * length(ds);
}

vec3 transmittance(vec3 from, vec3 to) {
    const vec3 integral = integrate_sigma_t(from, to);
    return exp(-integral);
}

/**
 * Compute atmospheric transmittance from ground to outer atmosphere
 * along the sun direction using physical Rayleigh, Mie and ozone scattering.
 */
vec3 compute_sun_transmittance(vec3 sun_dir) {
    vec3 origin = origin_view();
    float distance_out = intersectRaySphereFromInside(origin, sun_dir, radiusAtmo());
    vec3 out_atmosphere = origin + sun_dir * distance_out;
    return transmittance(origin, out_atmosphere);
}

void main()
{
    vec3 normal = texture(samplerNormal, inUV).rgb;
    vec3 position = texture(samplerPosition, inUV).rgb;
    vec3 albedo = texture(samplerColor, inUV).rgb;

    // Extract sky parameters
    vec3 star_direction = sky.star_direction_and_constant.xyz;
    vec3 star_color = sky.star_color_and_solid_angle.xyz;

    // Direction TO the sun (negated star direction which points FROM sun)
    vec3 L = normalize(-star_direction);
    float NdotL = max(dot(normal, L), 0.0);

    // Shadow ray tracing using ray query
    float shadow = 1.0;

    // Only trace shadow ray if surface faces the sun
    if (NdotL > 0.0) {
        // Offset along normal to avoid self-intersection
        // Use distance-adaptive bias for numerical stability at large world coordinates
        float bias = max(0.1, length(position) * 0.0005);
        vec3 rayOrigin = position;
        vec3 rayDirection = L;
        float tMin = 1;
        float tMax = 100001.0;

        rayQueryEXT rayQuery;
        rayQueryInitializeEXT(
            rayQuery,
            topLevelAS,
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
            0xFF,
            rayOrigin,
            tMin,
            rayDirection,
            tMax
        );

        // Traverse the acceleration structure
        while (rayQueryProceedEXT(rayQuery)) {
            // For opaque geometry, we don't need to do anything here
        }

        // Check if we hit anything
        if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
            shadow = 0.;
        }
    }

    // Sample ambient occlusion
    float ao = texture(samplerAO, inUV).r;

    // Compute atmospheric transmittance using physical Rayleigh, Mie and ozone
    vec3 atmo_transmittance = compute_sun_transmittance(L);

    // Compute sun radiance at ground level (W/m²/sr)
    // L_sun = (solar_constant / solid_angle) * transmittance * star_color
    vec3 L_sun = sunRadiance() * atmo_transmittance * star_color;

    // Convert to luminance units (cd/m²/sr) using luminous efficiency
    L_sun *= luminousEfficiency();

    // Physically-based lighting using radiance formulation:
    // L_out = BRDF * L_sun * solid_angle * cos(theta)
    // For Lambertian: BRDF = albedo / PI
    float solid_angle = starSolidAngle();
    vec3 diffuse = (albedo / PI) * L_sun * solid_angle * NdotL * shadow;

    // Add ambient term with AO (proportional to sun radiance)
    // Using ~10% as ambient contribution
    vec3 ambient = (albedo / PI) * L_sun * solid_angle * 0.1 * ao;

    // Output final luminance (cd/m²)
    outColor = vec4(diffuse + ambient, 1.0);
}
