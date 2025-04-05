#version 450

layout(set = 0, binding = 0, std140) uniform UBO {
    mat4 proj;
    mat4 view;
    mat4 model;
};

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = proj * view * model * vec4(inPosition, 1.0);
}