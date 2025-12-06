#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangeant;
layout(location = 3) in vec3 biTangeant;
layout(location = 4) in vec2 texCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outPosition;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outTangeant;
layout(location = 4) out vec3 outBiTangeant;

layout(set = 1, binding = 0, std140) uniform ColoredMaterial {
    vec4 inColor;    
};

void main()
{
    outColor = inColor;
    outPosition = position;
    outNormal = normal;
    outTangeant = tangeant;
    outBiTangeant = biTangeant;
}