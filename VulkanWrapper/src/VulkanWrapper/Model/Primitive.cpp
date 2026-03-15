#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Model/Primitive.h"

#include <cmath>

namespace vw::Model {

namespace {

struct AxisMapping {
    int u_axis;      // index into vec3 for U direction
    int v_axis;      // index into vec3 for V direction
    int normal_axis;  // index into vec3 for normal direction
};

AxisMapping get_axis_mapping(PlanePrimitive plane) {
    switch (plane) {
    case PlanePrimitive::XY:
        return {0, 1, 2}; // U=X, V=Y, Normal=Z
    case PlanePrimitive::XZ:
        return {0, 2, 1}; // U=X, V=Z, Normal=Y
    case PlanePrimitive::YZ:
        return {1, 2, 0}; // U=Y, V=Z, Normal=X
    }
    return {0, 1, 2};
}

glm::vec3 make_position(const AxisMapping &axes, float u, float v) {
    glm::vec3 pos{0.0f};
    pos[axes.u_axis] = u;
    pos[axes.v_axis] = v;
    return pos;
}

glm::vec3 make_normal(const AxisMapping &axes) {
    glm::vec3 n{0.0f};
    n[axes.normal_axis] = 1.0f;
    return n;
}

glm::vec3 make_tangent(const AxisMapping &axes) {
    glm::vec3 t{0.0f};
    t[axes.u_axis] = 1.0f;
    return t;
}

} // namespace

ModelPrimitiveResult create_triangle(PlanePrimitive plane) {
    auto axes = get_axis_mapping(plane);
    auto normal = make_normal(axes);
    auto tangeant = make_tangent(axes);
    auto bitangeant = glm::cross(normal, tangeant);

    // Equilateral triangle centered at origin.
    // Circumradius = 0.5 so it fits in [-0.5, 0.5].
    const float radius = 0.5f;

    // Vertices at 90, 210, 330 degrees (top, bottom-left,
    // bottom-right).
    std::array<glm::vec2, 3> coords;
    for (int i = 0; i < 3; ++i) {
        float angle =
            glm::radians(90.0f + 120.0f * static_cast<float>(i));
        coords[i] = {radius * std::cos(angle),
                      radius * std::sin(angle)};
    }

    std::vector<FullVertex3D> vertices;
    vertices.reserve(3);
    for (int i = 0; i < 3; ++i) {
        auto pos =
            make_position(axes, coords[i].x, coords[i].y);
        // Map positions from [-0.5, 0.5] to UVs in [0, 1].
        glm::vec2 uv{coords[i].x + 0.5f,
                      coords[i].y + 0.5f};
        vertices.emplace_back(pos, normal, tangeant, bitangeant,
                              uv);
    }

    return {std::move(vertices), {0, 1, 2}};
}

ModelPrimitiveResult create_square(PlanePrimitive plane) {
    auto axes = get_axis_mapping(plane);
    auto normal = make_normal(axes);
    auto tangeant = make_tangent(axes);
    auto bitangeant = glm::cross(normal, tangeant);

    // 4 corners of a unit square centered at origin.
    std::array<glm::vec2, 4> corners = {
        glm::vec2{-0.5f, -0.5f},
        glm::vec2{ 0.5f, -0.5f},
        glm::vec2{ 0.5f,  0.5f},
        glm::vec2{-0.5f,  0.5f},
    };

    std::array<glm::vec2, 4> uvs = {
        glm::vec2{0.0f, 0.0f},
        glm::vec2{1.0f, 0.0f},
        glm::vec2{1.0f, 1.0f},
        glm::vec2{0.0f, 1.0f},
    };

    std::vector<FullVertex3D> vertices;
    vertices.reserve(4);
    for (int i = 0; i < 4; ++i) {
        auto pos = make_position(axes, corners[i].x, corners[i].y);
        vertices.emplace_back(pos, normal, tangeant, bitangeant,
                              uvs[i]);
    }

    // Check if the V-axis direction disagrees with the
    // bitangent. When they point in opposite directions the
    // default index winding produces a geometric face normal
    // opposite to the declared normal, so we reverse the
    // triangles to fix backface culling.
    glm::vec3 v_axis{0.0f};
    v_axis[axes.v_axis] = 1.0f;
    bool flip = glm::dot(bitangeant, v_axis) < 0;

    if (flip) {
        return {std::move(vertices), {0, 2, 1, 0, 3, 2}};
    }
    return {std::move(vertices), {0, 1, 2, 0, 2, 3}};
}

ModelPrimitiveResult create_cube() {
    std::vector<FullVertex3D> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(24);
    indices.reserve(36);

    // Each face is defined by its normal direction, and two
    // tangent axes. We emit 4 vertices per face with correct
    // winding (CCW when viewed from outside).
    struct FaceDef {
        glm::vec3 normal;
        glm::vec3 right;  // tangent (U direction)
        glm::vec3 up;     // bitangent (V direction)
    };

    // 6 faces with outward normals.
    std::array<FaceDef, 6> faces = {{
        // +Z face (front)
        {{ 0,  0,  1}, { 1,  0,  0}, { 0,  1,  0}},
        // -Z face (back)
        {{ 0,  0, -1}, {-1,  0,  0}, { 0,  1,  0}},
        // +X face (right)
        {{ 1,  0,  0}, { 0,  0, -1}, { 0,  1,  0}},
        // -X face (left)
        {{-1,  0,  0}, { 0,  0,  1}, { 0,  1,  0}},
        // +Y face (top)
        {{ 0,  1,  0}, { 1,  0,  0}, { 0,  0, -1}},
        // -Y face (bottom)
        {{ 0, -1,  0}, { 1,  0,  0}, { 0,  0,  1}},
    }};

    for (const auto &face : faces) {
        auto base = static_cast<uint32_t>(vertices.size());

        // The 4 corners of the face, offset by 0.5 along
        // the normal direction.
        glm::vec3 center = face.normal * 0.5f;

        // Corners: bottom-left, bottom-right, top-right, top-left
        // in the face's local coordinate system.
        glm::vec3 bl = center - face.right * 0.5f
                               - face.up * 0.5f;
        glm::vec3 br = center + face.right * 0.5f
                               - face.up * 0.5f;
        glm::vec3 tr = center + face.right * 0.5f
                               + face.up * 0.5f;
        glm::vec3 tl = center - face.right * 0.5f
                               + face.up * 0.5f;

        auto bitangeant = glm::cross(face.normal, face.right);

        vertices.emplace_back(bl, face.normal, face.right,
                              bitangeant, glm::vec2{0, 0});
        vertices.emplace_back(br, face.normal, face.right,
                              bitangeant, glm::vec2{1, 0});
        vertices.emplace_back(tr, face.normal, face.right,
                              bitangeant, glm::vec2{1, 1});
        vertices.emplace_back(tl, face.normal, face.right,
                              bitangeant, glm::vec2{0, 1});

        // Two triangles, CCW winding.
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    return {std::move(vertices), std::move(indices)};
}

} // namespace vw::Model
