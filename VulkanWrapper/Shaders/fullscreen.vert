#version 450

layout (location = 0) out vec2 outUV;

void main() 
{
    // Triangle strip quad with counter-clockwise winding
    // Vertices: (0,0), (0,1), (1,0), (1,1)
    outUV = vec2((gl_VertexIndex >> 1) & 1, gl_VertexIndex & 1);
    gl_Position = vec4(outUV * 2.0f - 1.0f, 1.0f, 1.0f);
}
