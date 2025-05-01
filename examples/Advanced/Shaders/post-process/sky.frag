#version 450

layout(location = 0) in vec2 in_position;

layout(set = 0, binding = 0, std140) uniform Block {
    mat4 perspective;
    mat4 view;
    float angle;
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

const float PI = 3.14159265359;
const float Hr = 7994;
const float Hm = 1200;
const float Ho = 7994;

const vec3 rayleigh = vec3(5.8, 13.5, 33.1) * 1e-6;
const vec3 mie = vec3(21, 21, 21) * 1e-6;
const vec3 ozone = vec3(3.426, 8.298, 0.356) * 0.06 * 1e-5;

const float radiusEarth = 6360e3;
const float radiusAtmo = 6420e3;
const float ZenithH = radiusAtmo - radiusEarth;
const vec3 origin_view = vec3(0, radiusEarth + 1, 0);
const float originH = origin_view.y - radiusEarth;

const int STEPS = 16;

const float angular_size = 1.0 / 2.0;
const float cos_angular_size = cos(radians(angular_size));
const float cos_angular_bias = cos(radians(0.01));

float intersectRaySphereFromInside(const vec3 rayOrigin,
                                   const vec3 rayDir, float radius) {
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - radius * radius;
    float discriminant = b * b - c;
    float t = -b + sqrt(discriminant);
    return t;
}

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

vec3 sigma_t(vec3 position)
{
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

const vec3 TrZenith = exp(-(rayleigh * Hr * (exp(-originH / Hr) - exp(-ZenithH / Hr)) +
                            mie * Hm * (exp(-originH / Hm) - exp(-ZenithH / Hm)) + 
                            ozone * Ho * (exp(-originH / Ho) - exp(-ZenithH / Ho))));

vec3 LSun = PI * vec3(1.0, 0.9, 0.9) / TrZenith;

vec3 j(vec3 position, vec3 view_dir, vec3 sun_dir) {
    const float distance_out_atmosphere =
        intersectRaySphereFromInside(position, sun_dir, radiusAtmo);
    
    const vec3 out_atmosphere = position + sun_dir * distance_out_atmosphere;
    const vec3 trToSun = transmittance(position, out_atmosphere);
    const vec3 rayleigh_diffusion = sigma_s_rayleigh(position) * rayleigh_phase(view_dir, sun_dir);
    const vec3 mie_diffusion = sigma_s_mie(position) * mie_phase(view_dir, sun_dir);
    return LSun * trToSun * (rayleigh_diffusion + mie_diffusion);
}

vec3 compute_radiance(vec3 direction) {
    const vec3 sun_dir = normalize(vec3(cos(radians(angle)), sin(radians(angle)), 0.0));

    const float distance_out =
        intersectRaySphereFromInside(origin_view, direction, radiusAtmo);
    const vec3 view_out = origin_view + direction * distance_out;
    const float ds = distance_out / STEPS;

    float cos_theta = dot(direction, sun_dir);

    const float angle = degrees(acos(cos_theta));
    vec3 acc = vec3(0.0);
    float disk = 1.0 - smoothstep(angular_size * 0.95, angular_size * 1.05, angle);
    
    acc += disk * LSun * transmittance(origin_view, view_out);  

    for (int i = 0; i < STEPS; ++i) {
        const vec3 s = origin_view + (i + 0.5) * direction * ds;
        acc += ds * transmittance(origin_view, s) * j(s, direction, sun_dir);
    }

    return acc;
}

void main() {
    // Reconstruction direction caméra
    vec3 view_direction = get_view_direction();

    if(view_direction.y < 0) {
        out_color = vec4(0.1, 0.1, 0.1, 1.0);
        return;
    }
    vec3 color = compute_radiance(view_direction);

    out_color = vec4(color, 1.0);
}