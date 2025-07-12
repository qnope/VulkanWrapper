#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 payload;

void main() {
    // For shadow rays, we just need to know if we hit something
    // The payload will be used to determine shadowing
    payload = vec3(1.0); // Hit occurred
} 