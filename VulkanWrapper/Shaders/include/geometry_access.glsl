// Geometry Access - Buffer reference types for accessing vertex/index data in ray tracing shaders
// This file provides functions to access vertex data via device addresses.
//
// Usage: Define the geometry buffer binding before including this file:
//
//   #define GEOMETRY_BUFFER_BINDING 5
//   #include "geometry_access.glsl"
//
// In your hit shader:
//   hitAttributeEXT vec2 bary;
//   void main() {
//       VertexData v = interpolate_vertex(gl_InstanceCustomIndexEXT, gl_PrimitiveID, bary);
//       // Use v.position, v.normal, v.tangent, v.bitangent, v.uv
//       // Use v.material_type, v.material_index
//   }

#ifndef GEOMETRY_ACCESS_GLSL
#define GEOMETRY_ACCESS_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

// Verify required binding is defined
#ifndef GEOMETRY_BUFFER_BINDING
#error "GEOMETRY_BUFFER_BINDING must be defined before including geometry_access.glsl"
#endif

// FullVertex3D layout: position (vec3), normal (vec3), tangent (vec3), bitangent (vec3), uv (vec2)
// Total: 56 bytes per vertex
struct FullVertex3D {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 uv;
};

// Buffer reference for vertex data access via device address
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer FullVertexRef {
    FullVertex3D vertices[];
};

// Buffer reference for index data access via device address
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer IndexRef {
    uint indices[];
};

// GeometryReference matches the C++ struct in GeometryReference.h (96 bytes)
// Both uint64_t fields are first to ensure 8-byte alignment with scalar layout
struct GeometryReference {
    uint64_t vertex_buffer_address;
    uint64_t index_buffer_address;
    int vertex_offset;
    int first_index;
    uint material_type;
    uint material_index;
    mat4 matrix;
};

// Storage buffer containing geometry references for all meshes
layout(set = 0, binding = GEOMETRY_BUFFER_BINDING, scalar) readonly buffer GeometryReferenceBuffer {
    GeometryReference geometry_refs[];
};

// Interpolated vertex data returned by interpolate_vertex
struct VertexData {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 uv;
    uint material_type;
    uint material_index;
};

// Get the three vertices of a triangle
void get_triangle_vertices(uint geometry_index, uint primitive_id,
                           out FullVertex3D v0, out FullVertex3D v1, out FullVertex3D v2) {
    GeometryReference geom = geometry_refs[geometry_index];

    FullVertexRef vertex_buffer = FullVertexRef(geom.vertex_buffer_address);
    IndexRef index_buffer = IndexRef(geom.index_buffer_address);

    uint i0 = index_buffer.indices[geom.first_index + primitive_id * 3 + 0];
    uint i1 = index_buffer.indices[geom.first_index + primitive_id * 3 + 1];
    uint i2 = index_buffer.indices[geom.first_index + primitive_id * 3 + 2];

    v0 = vertex_buffer.vertices[geom.vertex_offset + i0];
    v1 = vertex_buffer.vertices[geom.vertex_offset + i1];
    v2 = vertex_buffer.vertices[geom.vertex_offset + i2];
}

// Interpolate vertex attributes using barycentric coordinates
// geometry_index: gl_InstanceCustomIndexEXT
// primitive_id: gl_PrimitiveID
// bary: barycentric coordinates (hitAttributeEXT vec2)
VertexData interpolate_vertex(uint geometry_index, uint primitive_id, vec2 bary) {
    FullVertex3D v0, v1, v2;
    get_triangle_vertices(geometry_index, primitive_id, v0, v1, v2);

    float w0 = 1.0 - bary.x - bary.y;
    float w1 = bary.x;
    float w2 = bary.y;

    VertexData result;
    result.position = v0.position * w0 + v1.position * w1 + v2.position * w2;
    result.normal = normalize(v0.normal * w0 + v1.normal * w1 + v2.normal * w2);
    result.tangent = normalize(v0.tangent * w0 + v1.tangent * w1 + v2.tangent * w2);
    result.bitangent = normalize(v0.bitangent * w0 + v1.bitangent * w1 + v2.bitangent * w2);
    result.uv = v0.uv * w0 + v1.uv * w1 + v2.uv * w2;

    GeometryReference geom = geometry_refs[geometry_index];
    result.material_type = geom.material_type;
    result.material_index = geom.material_index;

    return result;
}

// Convenience functions for common use cases

// Get interpolated hit position
vec3 get_hit_position(uint geometry_index, uint primitive_id, vec2 bary) {
    FullVertex3D v0, v1, v2;
    get_triangle_vertices(geometry_index, primitive_id, v0, v1, v2);

    float w0 = 1.0 - bary.x - bary.y;
    float w1 = bary.x;
    float w2 = bary.y;

    return v0.position * w0 + v1.position * w1 + v2.position * w2;
}

// Get interpolated and normalized hit normal
vec3 get_hit_normal(uint geometry_index, uint primitive_id, vec2 bary) {
    FullVertex3D v0, v1, v2;
    get_triangle_vertices(geometry_index, primitive_id, v0, v1, v2);

    float w0 = 1.0 - bary.x - bary.y;
    float w1 = bary.x;
    float w2 = bary.y;

    return normalize(v0.normal * w0 + v1.normal * w1 + v2.normal * w2);
}

// Get interpolated UV coordinates
vec2 get_hit_uv(uint geometry_index, uint primitive_id, vec2 bary) {
    FullVertex3D v0, v1, v2;
    get_triangle_vertices(geometry_index, primitive_id, v0, v1, v2);

    float w0 = 1.0 - bary.x - bary.y;
    float w1 = bary.x;
    float w2 = bary.y;

    return v0.uv * w0 + v1.uv * w1 + v2.uv * w2;
}

// Get interpolated TBN matrix (tangent, bitangent, normal)
mat3 get_hit_tbn(uint geometry_index, uint primitive_id, vec2 bary) {
    FullVertex3D v0, v1, v2;
    get_triangle_vertices(geometry_index, primitive_id, v0, v1, v2);

    float w0 = 1.0 - bary.x - bary.y;
    float w1 = bary.x;
    float w2 = bary.y;

    vec3 tangent = normalize(v0.tangent * w0 + v1.tangent * w1 + v2.tangent * w2);
    vec3 bitangent = normalize(v0.bitangent * w0 + v1.bitangent * w1 + v2.bitangent * w2);
    vec3 normal = normalize(v0.normal * w0 + v1.normal * w1 + v2.normal * w2);

    return mat3(tangent, bitangent, normal);
}

#endif // GEOMETRY_ACCESS_GLSL
