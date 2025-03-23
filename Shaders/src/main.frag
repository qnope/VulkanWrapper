#version 450 core

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 n;
layout(location = 2) in vec3 t;
layout(location = 3) in vec3 b;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D image;

void main()
{
    float mipmapLevel = textureQueryLod(image, texCoord).x;
    outColor = textureLod(image, texCoord, mipmapLevel);
}