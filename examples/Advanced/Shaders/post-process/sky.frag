#version 450

layout(location = 0) in vec2 in_position;

layout(set = 0, binding = 0, std140) uniform Block {
    mat4 perspective;
    mat4 view;
};

layout(location = 0) out vec4 out_color;

vec3 get_world_pos(vec4 clipPosition)
{
    mat4 invMatrix = inverse(perspective * view);
    vec4 world = invMatrix * clipPosition;
    world /= world.w;
    return world.xyz;
}

vec3 get_view_direction() {
    vec3 near = get_world_pos(vec4(in_position, 0.0, 1.0));
    vec3 far = get_world_pos(vec4(in_position, 1.0, 1.0));
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
const float Hr = 8000;
const float Hm = 1200;
const float Ho = 8000;

const vec3 rayleigh = vec3(5.8, 13.5, 33.1) * 1e-6;
const vec3 mie = vec3(21, 21, 21) * 1e-6;
const vec3 ozone = vec3(3.426, 8.298, 0.356) * 0.06 * 1e-5;
const float radiusEarth = 6360e3;
const float radiusAtmo = 6420e3;
const float ZenithH = radiusAtmo - radiusEarth;
const vec3 origin_view = vec3(0, radiusEarth + 1, 0);
const float originH = origin_view.y - radiusEarth;
const int STEPS = 8;
const float angle = 30.0;

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
    const float h = length(position) - radiusEarth;
    return rayleigh * exp(-h / Hr);
}

vec3 sigma_s_mie(vec3 position) {
    const float h = length(position) - radiusEarth;
    return mie * exp(-h / Hm);
}

vec3 sigma_a_ozone(vec3 position) {
    const float h = length(position) - radiusEarth;
    return ozone * exp(-h / Ho);
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

const float radius_size = radians(0.53 / 2.0);
const float sun_solid_angle = 2 * PI * (1 - cos(radius_size));

//vec3 TrZenith = transmittance(origin_view, vec3(0, radiusEarth + ZenithH, 0));
const vec3 TrZenith = exp(-(rayleigh * Hr * (exp(-originH / Hr) - exp(-ZenithH / Hr)) +
                            mie * Hm * (exp(-originH / Hm) - exp(-ZenithH / Hm)) + 
                            ozone * Ho * (exp(-originH / Ho) - exp(-ZenithH / Ho))));

vec3 illuminanceGround = 2 * PI * (vec3(1.0, 1.0, 1.0));
vec3 L_outerspace = (illuminanceGround / sun_solid_angle) / TrZenith;

vec3 j(vec3 position, vec3 view_dir, vec3 sun_dir) {
    const float distance_out_atmosphere =
        intersectRaySphereFromInside(position, sun_dir, radiusAtmo);
    const vec3 out_atmosphere = position + sun_dir * distance_out_atmosphere;
    
    const vec3 trToSun = transmittance(position, out_atmosphere);
    const vec3 rayleigh_diffusion = sigma_s_rayleigh(position) * rayleigh_phase(view_dir, sun_dir);
    const vec3 mie_diffusion = sigma_s_mie(position) * mie_phase(view_dir, sun_dir);
    
    return L_outerspace * trToSun * sun_solid_angle * (rayleigh_diffusion + mie_diffusion);
}

vec3 compute_luminance(vec3 out_atmosphere, vec3 sun_dir) {
    const vec3 ds = (out_atmosphere - origin_view) / STEPS;
    const vec3 direction = normalize(ds);
    vec3 acc = vec3(0.0);

    for (int i = 0; i < STEPS; ++i) {
        const vec3 s = origin_view + (i + 0.5) * ds;
        acc += transmittance(origin_view, s) * j(s, direction, sun_dir);
    }

    return acc * length(ds);
}

vec3 direct_light_from_sun(vec3 direction, vec3 out_atmosphere, vec3 sun_dir) {
    float cos_theta = dot(direction, sun_dir);

    const float angle = acos(cos_theta);
    float disk = 1.0 - smoothstep(radius_size * 0.95, radius_size * 1.05, angle);

    return disk * L_outerspace * transmittance(origin_view, out_atmosphere); 
}

void main() {
    const vec3 view_direction = get_view_direction();
    const vec3 sun_dir = normalize(vec3(cos(radians(angle)), sin(radians(angle)), 0.0));

    const float distance_out =
        intersectRaySphereFromInside(origin_view, view_direction, radiusAtmo);
    const vec3 view_out = origin_view + view_direction * distance_out;

    vec3 luminance = compute_luminance(view_out, sun_dir);
    luminance += direct_light_from_sun(view_direction, view_out, sun_dir);

    out_color = vec4(luminance, 1.0);
    //out_color = vec4(1.0, 1.0, 1.0, 1.0);
}