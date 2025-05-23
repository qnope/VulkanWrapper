#version 450

layout(location = 0) out vec2 outPosition;

void main() {
    // Triangle strip avec 4 sommets (0 à 3)
    const vec2 positions[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0, -1.0), 
        vec2( 1.0,  1.0)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 1.0, 1.0);
    outPosition = positions[gl_VertexIndex];
}