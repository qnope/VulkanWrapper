#version 450 core

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D image;

void main()
{
    outColor = texture(image, texCoord);
}