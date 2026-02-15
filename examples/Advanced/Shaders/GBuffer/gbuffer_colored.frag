#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 tangeant;
layout(location = 2) in vec3 biTangeant;
layout(location = 3) in vec2 texCoord;
layout(location = 4) in vec3 worldPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangeant;
layout(location = 3) out vec3 outBiTangeant;
layout(location = 4) out vec4 outPosition;
layout(location = 5) out vec4 outIndirectRay;

// Random sampling with Cranley-Patterson rotation
#define RANDOM_XI_BUFFER_BINDING 1
#define RANDOM_NOISE_TEXTURE_BINDING 2
#include "random.glsl"

// Buffer reference for colored material data
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer ColoredMaterialRef {
    vec3 color;
};

layout(push_constant, scalar) uniform PushConstants {
    mat4 model;
    uint64_t materialAddress;
    uint frame_count;
};

void main()
{
    ColoredMaterialRef mat = ColoredMaterialRef(materialAddress);
    outColor = vec4(mat.color, 1.0);
    outNormal = normal;
    outTangeant = tangeant;
    outBiTangeant = biTangeant;
    outPosition = vec4(worldPosition, 1.0);

    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 xi = get_sample(frame_count, pixel);
    vec3 N = normalize(normal);
    vec3 T = normalize(tangeant);
    vec3 B = normalize(biTangeant);
    vec3 ray_dir = sample_hemisphere_cosine(N, T, B, xi);
    outIndirectRay = vec4(ray_dir, 1.0);
}
