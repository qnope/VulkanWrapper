#version 450

layout(set = 0, binding = 0, std140) uniform UBO {
    mat4 proj;
    mat4 view;
    mat4 model;
};

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 texCoord;

void main() {
    gl_Position = proj * view * model * vec4(inPosition, 0.0, 1.0);
    fragColor = vec4(inColor, 1.0);
    texCoord = inTexCoord;
}