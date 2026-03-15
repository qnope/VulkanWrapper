#version 460

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec3 color;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(pc.color, 1.0);
}
