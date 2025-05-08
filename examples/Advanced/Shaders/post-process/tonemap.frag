#version 450

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput base_color;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput radiance;

layout(location = 0) out vec4 out_color;

vec4 ACESApprox(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return vec4(clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0), 1.0);
}

float luminanceFromRGB(vec3 rgb_cd_per_m2) {
    const vec3 lumaCoeffs = vec3(0.2126, 0.7152, 0.0722);
    
    return dot(rgb_cd_per_m2, lumaCoeffs); // cd/m²
}

vec3 heatLuminance(float luminance) {
    float t = clamp(luminance / 1e7, 0.0, 1.0);

    vec3 blue = vec3(0.0, 0.0, 1.0);
    vec3 green = vec3(0.0, 1.0, 0.0);
    vec3 red = vec3(1.0, 0.0, 0.0);

    if (t < 0.5) {
        return mix(blue, green, t * 2.0);
    } else {
        return mix(green, red, (t - 0.5) * 2.0);
    }
}

void main() {
    vec3 base = subpassLoad(base_color).rgb;
    vec3 rad = subpassLoad(radiance).rgb;

    const vec3 exposed = base * rad;
    out_color = ACESApprox(exposed);
}