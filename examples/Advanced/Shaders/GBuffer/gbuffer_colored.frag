#version 450 core

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

// Material data SSBO
layout(set = 1, binding = 0, std430) readonly buffer ColoredMaterials {
    vec4 colors[];
} coloredMaterials;

void main()
{
    outColor = coloredMaterials.colors[materialIndex];
    outNormal = normal;
    outTangeant = tangeant;
    outBiTangeant = biTangeant;
    outPosition = vec4(worldPosition, 1.0);
}
