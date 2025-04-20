#version 450

layout(location = 0) in vec2 in_position;

layout(set = 0, binding = 0, std140) uniform Block {
    mat4 perspective;
    mat4 view;

};

layout(location = 0) out vec4 out_color;

void main() {
    const float dateTime = 0.5;
    float cloudiness = 0.5;

    out_color = vec4(0.0, 0.0, 1.0, 1.0);
}