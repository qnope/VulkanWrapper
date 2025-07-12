#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 payload;

void main() {
    // No hit occurred, so no shadow
    payload = vec3(0.0); // No shadow
} 