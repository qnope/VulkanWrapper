#version 450

layout(location = 0) in vec2 in_position;

// SkyParametersGPU + inverseViewProj as push constants
layout(push_constant) uniform PushConstants {
    vec4 star_direction_and_constant;   // xyz = direction FROM star, w = constant
    vec4 star_color_and_solid_angle;    // xyz = color, w = solid_angle
    vec4 rayleigh_and_height_r;         // xyz = rayleigh_coef, w = height_rayleigh
    vec4 mie_and_height_m;              // xyz = mie_coef, w = height_mie
    vec4 ozone_and_height_o;            // xyz = ozone_coef, w = height_ozone
    vec4 radii_and_efficiency;          // x = radius_planet, y = radius_atmosphere, z = luminous_efficiency
    mat4 inverseViewProj;
} sky;

layout(location = 0) out vec4 out_color;

vec3 get_world_pos(vec4 clipPosition)
{
    vec4 world = sky.inverseViewProj * clipPosition;
    world /= world.w;
    return world.xyz;
}

vec3 get_view_direction() {
    // Convert UV coordinates [0,1] to clip space [-1,1]
    vec2 clip_pos = in_position * 2.0 - 1.0;
    vec3 near = get_world_pos(vec4(clip_pos, 0.0, 1.0));
    vec3 far = get_world_pos(vec4(clip_pos, 1.0, 1.0));
    return normalize(far - near);
}

float intersectRaySphereFromInside(const vec3 rayOrigin,
                                   const vec3 rayDir, float radius) {
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - radius * radius;
    float discriminant = b * b - c;
    float t = -b + sqrt(discriminant);
    return t;
}

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

// Derived constants
float ZenithH() { return radiusAtmo() - radiusEarth(); }
vec3 origin_view() { return vec3(0, radiusEarth() + 1, 0); }
float originH() { return 1.0; }

float rayleigh_phase(vec3 view_dir, vec3 sun_dir) {
    const float mu = dot(view_dir, sun_dir);
    return (3.0 / (16.0 * PI)) * (1.f + mu * mu);
}

float mie_phase(vec3 view_dir, vec3 sun_dir) {
    float mu = dot(view_dir, sun_dir);
    float g = 0.76;
    float denom = 1.0 + g * g - 2.0 * g * mu;
    return (1.0 - g * g) / (4.0 * PI * pow(denom, 1.5));
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

// Compute sun disk radiance from solar constant and solid angle
// L = E / Ω (radiance = irradiance / solid_angle)
vec3 L_outerspace() {
    return vec3(starConstant() / starSolidAngle());
}

vec3 j(vec3 position, vec3 view_dir, vec3 sun_dir) {
    const float distance_out_atmosphere =
        intersectRaySphereFromInside(position, sun_dir, radiusAtmo());
    const vec3 out_atmosphere = position + sun_dir * distance_out_atmosphere;

    const vec3 trToSun = transmittance(position, out_atmosphere);
    const vec3 rayleigh_diffusion = sigma_s_rayleigh(position) * rayleigh_phase(view_dir, sun_dir);
    const vec3 mie_diffusion = sigma_s_mie(position) * mie_phase(view_dir, sun_dir);

    return L_outerspace() * trToSun * starSolidAngle() * (rayleigh_diffusion + mie_diffusion);
}

vec3 compute_luminance(vec3 out_atmosphere, vec3 sun_dir) {
    vec3 origin = origin_view();
    const vec3 ds = (out_atmosphere - origin) / STEPS;
    const vec3 direction = normalize(ds);
    vec3 acc = vec3(0.0);

    for (int i = 0; i < STEPS; ++i) {
        const vec3 s = origin + (i + 0.5) * ds;
        acc += transmittance(origin, s) * j(s, direction, sun_dir);
    }

    return acc * length(ds);
}

vec3 direct_light_from_sun(vec3 direction, vec3 out_atmosphere, vec3 sun_dir) {
    float cos_theta = dot(direction, sun_dir);

    // Sun disk angular radius from solid angle
    // solid_angle = 2*PI*(1 - cos(theta))
    // cos(theta) = 1 - solid_angle/(2*PI)
    // theta = acos(1 - solid_angle/(2*PI))
    float radius_size = acos(1.0 - starSolidAngle() / (2.0 * PI));
    const float angle = acos(cos_theta);
    float disk = 1.0 - smoothstep(radius_size * 0.95, radius_size * 1.05, angle);

    return disk * L_outerspace() * transmittance(origin_view(), out_atmosphere);
}

void main() {
    const vec3 view_direction = get_view_direction();
    // Direction TO the sun (opposite of star_direction which is FROM star)
    const vec3 sun_dir = normalize(-sky.star_direction_and_constant.xyz);

    vec3 origin = origin_view();
    const float distance_out =
        intersectRaySphereFromInside(origin, view_direction, radiusAtmo());
    const vec3 view_out = origin + view_direction * distance_out;

    vec3 radiance = compute_luminance(view_out, sun_dir);
    radiance += direct_light_from_sun(view_direction, view_out, sun_dir);

    // Convert from W/m²/sr to cd/m² using luminous efficiency
    vec3 luminance = radiance * luminousEfficiency();

    out_color = vec4(luminance, 1.0);
}
