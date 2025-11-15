#version 460 core
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec4 payload;

layout(shaderRecordEXT) buffer sbt
{
    vec3 color;
};

void main() {
    payload = vec4(color, 1.0);
} 