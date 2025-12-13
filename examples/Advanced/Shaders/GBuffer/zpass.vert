#version 450

layout(set = 0, binding = 0, std140) uniform UBO {
    mat4 proj;
    mat4 view;
};

layout(push_constant) uniform PushConstants {
    mat4 model;
};

layout(location = 0) in vec3 inPosition;

void main() {
    // Compute gl_Position in the same order as gbuffer.vert to ensure
    // identical depth values for the eEqual depth test in ColorPass
    vec4 worldPos = model * vec4(inPosition, 1.0);
    gl_Position = proj * view * worldPos;
}