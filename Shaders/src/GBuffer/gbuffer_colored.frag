#version 450 core

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0, std140) uniform ColoredMaterial {
    vec4 inColor;    
};

void main()
{
    outColor = inColor;
}