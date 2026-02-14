#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : require

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

// Buffer reference for textured material data
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer TexturedMaterialRef {
    uint diffuse_texture_index;
};

layout(push_constant, scalar) uniform PushConstants {
    mat4 model;
    uint64_t materialAddress;
};

// Bindless texture descriptor set
layout(set = 1, binding = 0) uniform sampler globalSampler;
layout(set = 1, binding = 1) uniform texture2D textures[];

void main()
{
    TexturedMaterialRef mat = TexturedMaterialRef(materialAddress);
    uint texIdx = mat.diffuse_texture_index;
    float mipmapLevel = textureQueryLod(sampler2D(textures[nonuniformEXT(texIdx)], globalSampler), texCoord).x;
    outColor = textureLod(sampler2D(textures[nonuniformEXT(texIdx)], globalSampler), texCoord, mipmapLevel);
    outNormal = normal;
    outTangeant = tangeant;
    outBiTangeant = biTangeant;
    outPosition = vec4(worldPosition, 1.0);
}
