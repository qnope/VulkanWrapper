#version 450

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput base_color;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput radiance;

layout(location = 0) out vec4 out_color;

void main() {
    vec4 base = subpassLoad(base_color);
    vec4 rad = subpassLoad(radiance);

    out_color = base * rad;
}