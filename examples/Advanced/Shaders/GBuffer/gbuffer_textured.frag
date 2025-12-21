#version 450 core
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 tangeant;
layout(location = 2) in vec3 biTangeant;
layout(location = 3) in vec2 texCoord;
layout(location = 4) in vec3 worldPosition;
layout(location = 5) flat in uint materialIndex;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangeant;
layout(location = 3) out vec3 outBiTangeant;
layout(location = 4) out vec4 outPosition;

// Bindless descriptor set
// Note: binding order matters for variable descriptor count - textures must be last
layout(set = 1, binding = 0) uniform sampler globalSampler;
layout(set = 1, binding = 1, std430) readonly buffer TexturedMaterials {
    uvec4 materials[];  // x = diffuse_texture_index, yzw = padding
} texturedMaterials;
layout(set = 1, binding = 2) uniform texture2D textures[];

void main()
{
    uint texIdx = texturedMaterials.materials[materialIndex].x;
    float mipmapLevel = textureQueryLod(sampler2D(textures[nonuniformEXT(texIdx)], globalSampler), texCoord).x;
    outColor = textureLod(sampler2D(textures[nonuniformEXT(texIdx)], globalSampler), texCoord, mipmapLevel);
    outNormal = normal;
    outTangeant = tangeant;
    outBiTangeant = biTangeant;
    outPosition = vec4(worldPosition, 1.0);
}
