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

const float dateTime = 0.5;
const float cloudiness = 0.5;

const float PI = 3.14159265359;
float Hr = 7994;
float Hm = 1200;
const vec3 rayleigh = vec3(5.8e-6, 13.5e-6, 33.1e-6);
const vec3 mie = vec3(21e-6, 21e-6, 21e-6);
const float radiusEarth = 6360e3;
const float radiusAtmo = 6420e3;
const vec3 origin_view = vec3(0, radiusEarth + 10, 0);
const int STEPS = 16;

float rayleigh_phase(vec3 view_dir, vec3 sun_dir) {
    const float mu = dot(view_dir, sun_dir);
    return (3.0 / (16.0 * PI)) * (1.f + mu * mu);
}

float mie_phase(vec3 view_dir, vec3 sun_dir) {
    const float mu = dot(view_dir, sun_dir);
    const float g = 0.76;
    return 3.f / (8.f * PI) * ((1.f - g * g) * (1.f + mu * mu)) / ((2.f + g * g) * pow(1.f + g * g - 2.f * g * mu, 1.5f)); 
}

vec3 sigma_r(vec3 position) {
    const float h = length(position) - radiusEarth;
    return rayleigh * exp(-h / Hr);
}

vec3 sigma_m(vec3 position) {
    const float h = length(position) - radiusEarth;
    return mie * exp(-h / Hm);
}

vec3 integrate_sigma_r(vec3 from, vec3 to) {
    const vec3 dt = (to - from) / float(STEPS);
    vec3 accumulation = vec3(0, 0, 0);

    for (int i = 0; i < STEPS; ++i) {
        const vec3 s = from + dt * 0.5;
        accumulation += sigma_r(s);
        from += dt;
    }

    return accumulation * length(dt);
}

vec3 integrate_sigma_m(vec3 from, vec3 to) {
    const vec3 dt = (to - from) / float(STEPS);
    vec3 accumulation = vec3(0, 0, 0);

    for (int i = 0; i < STEPS; ++i) {
        const vec3 s = from + dt * 0.5;
        accumulation += sigma_m(s);
        from += dt;
    }

    return accumulation * length(dt);
}

vec3 integrate_sigma(vec3 from, vec3 to) {
    const vec3 dt = (to - from) / float(STEPS);
    vec3 accumulation = vec3(0, 0, 0);

    for (int i = 0; i < STEPS; ++i) {
        const vec3 s = from + dt * 0.5;
        accumulation += sigma_m(s) + sigma_r(s);
        from += dt;
    }

    return accumulation * length(dt);
}

vec3 C(vec3 from, vec3 to) {
    const vec3 integral = integrate_sigma(from, to);
    return exp(-integral);
}

float intersectRaySphereFromInside(const vec3 rayOrigin,
                                   const vec3 rayDir, float radius) {
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - radius * radius;
    float discriminant = b * b - c;

    float t = -b + sqrt(discriminant);
    return t;
}

vec3 Br(vec3 position) {
    const vec3 sun_dir = normalize(vec3(cos(radians(angle)), sin(radians(angle)), 0.0));
    const float distance_out_sun =
        intersectRaySphereFromInside(position, sun_dir, radiusAtmo);
    const vec3 to = position + sun_dir * distance_out_sun;

    const vec3 integral = integrate_sigma_r(position, to);

    return exp(-integral);
}

vec3 Bm(vec3 position) {
    const vec3 sun_dir = normalize(vec3(cos(radians(angle)), sin(radians(angle)), 0.0));
    const float distance_out_sun =
        intersectRaySphereFromInside(position, sun_dir, radiusAtmo);
    const vec3 to = position + sun_dir * distance_out_sun;

    const vec3 integral = integrate_sigma_m(position, to);

    return exp(-integral);
}

vec3 compute_radiance(vec3 direction) {
    const vec3 sun_dir = normalize(vec3(cos(radians(angle)), sin(radians(angle)), 0.0));

    const float distance_out =
        intersectRaySphereFromInside(origin_view, direction, radiusAtmo);

    const vec3 view_out = origin_view + direction * distance_out;
    const float dt = distance_out / STEPS;
    vec3 origin = origin_view;

    vec3 acc = vec3(0, 0, 0);
    for (int i = 0; i < STEPS; ++i) {
        const vec3 s = origin + direction * dt * 0.5;

        const vec3 trToSky = C(origin_view, s);

        const vec3 trToSun = Br(s) * Bm(s);

        acc += trToSky * trToSun *
        ((rayleigh_phase(direction, sun_dir) * sigma_r(s)) + 
        (mie_phase(direction, sun_dir) * sigma_m(s)));
        origin += direction * dt;
    }

    return acc * dt * 5;
}

void main() {
    // Reconstruction direction caméra
    vec3 view_direction = get_view_direction();

    if(view_direction.y < 0)
        return;
    vec3 color = compute_radiance(view_direction);

    out_color = vec4(color, 1.0);
}

/*
void main() {
    const vec3 sun_dir = normalize(vec3(cos(radians(angle)), sin(radians(angle)), 0.0));
    vec3 view_direction = get_view_direction();
    float t1 = intersectRaySphereFromInside(origin_view, view_direction, radiusAtmo);
    uint numSamples = 16; 
    uint numSamplesLight = 8; 
    float segmentLength = t1 / numSamples; 
    float tCurrent = 0.0; 
    vec3 sumR = vec3(0.0), sumM = vec3(0.0);  //mie and rayleigh contribution 
    float opticalDepthR = 0, opticalDepthM = 0; 
    float mu = dot(view_direction, sun_dir);  //mu in the paper which is the cosine of the angle between the sun direction and the ray direction 
    float phaseR = rayleigh_phase(view_direction, sun_dir); 
    float g = 0.76f; 
    float phaseM = mie_phase(view_direction, sun_dir);
    for (uint i = 0; i < numSamples; ++i) { 
        vec3 samplePosition = origin_view + (tCurrent + segmentLength * 0.5) * view_direction; 
        float height = length(samplePosition) - radiusEarth; 
        // compute optical depth for light
        float hr = exp(-height / Hr) * segmentLength; 
        float hm = exp(-height / Hm) * segmentLength; 
        opticalDepthR += hr; 
        opticalDepthM += hm; 
        // light optical depth
        float t1Light = intersectRaySphereFromInside(samplePosition, sun_dir, radiusAtmo); 
        float segmentLengthLight = t1Light / numSamplesLight, tCurrentLight = 0; 
        float opticalDepthLightR = 0, opticalDepthLightM = 0; 
        uint j; 
        for (j = 0; j < numSamplesLight; ++j) { 
            vec3 samplePositionLight = samplePosition + (tCurrentLight + segmentLengthLight * 0.5) * sun_dir; 
            float heightLight = length(samplePositionLight) - radiusEarth; 
            opticalDepthLightR += exp(-heightLight / Hr) * segmentLengthLight; 
            opticalDepthLightM += exp(-heightLight / Hm) * segmentLengthLight; 
            tCurrentLight += segmentLengthLight; 
        } 
        vec3 tauCam = rayleigh * (opticalDepthR) + mie * (opticalDepthM); 
        vec3 tauSun = rayleigh * (opticalDepthLightR) + mie * (opticalDepthLightM); 
        vec3 attenuation = exp(-tauSun) * exp(-tauCam); 
        sumR += attenuation * hr; 
        sumM += attenuation * hm; 
        
        tCurrent += segmentLength; 
    } 
 
    // We use a magic number here for the intensity of the sun (20). We will make it more
    // scientific in a future revision of this lesson/code
    out_color = vec4((sumR * rayleigh * phaseR + sumM * mie * phaseM) * 10, 1.0); 
}*/