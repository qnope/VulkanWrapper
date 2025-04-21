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

vec3 acesTonemap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Perlin Noise helpers
float hash(vec2 p) {
    return fract(sin(dot(p ,vec2(127.1,311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f*f*(3.0-2.0*f);
    return mix(mix(hash(i + vec2(0.0,0.0)), 
                   hash(i + vec2(1.0,0.0)), u.x),
               mix(hash(i + vec2(0.0,1.0)), 
                   hash(i + vec2(1.0,1.0)), u.x), u.y);
}

float fbm(vec2 p) {
    float f = 0.0;
    float w = 0.5;
    for (int i = 0; i < 6; i++) {
        f += w * noise(p);
        p *= 2.0;
        w *= 0.5;
    }
    return f;
}

const float dateTime = 0.3;
const float cloudiness = 0.5;

const float PI = 3.14159265359;

// Rayleigh coefficients (approx. atmosphere)
const vec3 betaRayleigh = vec3(5.8e-6, 13.5e-6, 33.1e-6); // R, G, B

// Mie scattering parameters
const vec3 betaMie = vec3(21e-6); // Gris blanchâtre
const float g = 0.76; // Anisotropie de Mie (halo directionnel)

// Rayleigh phase
float rayleighPhase(float cosTheta) {
    return (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
}

// Mie phase (Henyey-Greenstein)
float miePhase(float cosTheta, float g) {
    float g2 = g * g;
    return (3.0 / (8.0 * PI)) * ((1.0 - g2) * (1.0 + cosTheta * cosTheta)) / pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
}

// Soleil directionnel basé sur u_time01
vec3 getSunDirection(float time01) {
    float theta = mix(PI * 0.1, PI * 0.9, time01); // 0.5 = midi zenith, 0.0/1.0 = horizon
    return normalize(vec3(cos(theta), sin(theta), 0.0));
}

vec3 computeSkyColor(vec3 view_direction) {
    vec3 sunDir = getSunDirection(dateTime);
    float mu = dot(view_direction, sunDir); // cos(angle vue / soleil)

    // Phases
    float rPhase = rayleighPhase(mu);
    float mPhase = miePhase(mu, g);

    // Hauteur de vue influence la teinte (zénith vs horizon)
    float zenith = clamp(view_direction.y, 0.0, 1.0);
    float horizonFactor = pow(1.0 - zenith, 2.0);

    // Intensités
    float sunIntensity = max(dot(sunDir, vec3(0.0, 1.0, 0.0)), 0.0);
    vec3 rayleigh = betaRayleigh * rPhase * sunIntensity;
    vec3 mie = betaMie * mPhase * sunIntensity;

    // Ajoute du rouge/jaune à l'horizon
    vec3 horizonTint = mix(vec3(1.0, 0.6, 0.2), vec3(1.0), zenith);
    rayleigh *= horizonTint;

    // Halo solaire plus intense
    float sunHalo = exp(-pow(acos(mu) / 0.015, 2.0)); // 0.015 = taille du halo
    vec3 sunColor = vec3(1.0, 0.9, 0.6) * sunHalo * 10.0;

    vec3 ambientSky = vec3(0.1, 0.15, 0.25) * (1.0 - zenith); // gradient vertical

    float sunAngularSize = radians(0.53);
    float sunDisk = smoothstep(sunAngularSize * 0.5, 0.0, acos(mu));
    vec3 sunBody = vec3(1.0, 0.95, 0.8) * sunDisk * 50.0;

    // Radiance
    return rayleigh * 100000.0 + mie * 300.0 + sunColor + ambientSky * 0.5 + sunBody;
}

void main() {
    // Reconstruction direction caméra
    vec3 view_direction = get_view_direction();
    
    // Nuages via FBM
    vec2 cloudUV = view_direction.xz * 0.5 + vec2(0.5); // projection dans le ciel

    float cloudNoise = fbm(cloudUV * 30.0);
    float cloudDensity = smoothstep(0.5, 0.8, cloudNoise); // seuil pour rendre les bords doux

    // Teinte blanche des nuages + modulateur léger de ciel
    vec3 cloudColor = mix(vec3(0.0), vec3(1.0), cloudDensity);

    vec3 color = computeSkyColor(view_direction);

    color = mix(color, cloudColor, cloudDensity * 0.4);

    // sortie rgb
    out_color = vec4(acesTonemap(color), 1.0);
}