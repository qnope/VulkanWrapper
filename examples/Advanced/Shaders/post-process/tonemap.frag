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

void main() {
    vec3 base = subpassLoad(base_color).rgb;
    vec3 rad = subpassLoad(radiance).rgb;

    out_color = ACESApprox(base * rad);
}