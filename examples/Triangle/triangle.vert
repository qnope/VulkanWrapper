#version 460

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec3 color;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inUV;

void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
}
