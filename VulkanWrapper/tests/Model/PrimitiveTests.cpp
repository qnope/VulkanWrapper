#include <gtest/gtest.h>

#include <VulkanWrapper/Model/Primitive.h>

#include <cmath>
#include <set>

using namespace vw::Model;

constexpr float kEps = 1e-5f;

// ── Helpers ──────────────────────────────────────────────────

static void check_bounding_box(
    const std::vector<vw::FullVertex3D> &verts) {
    for (const auto &v : verts) {
        EXPECT_GE(v.position.x, -0.5f - kEps);
        EXPECT_LE(v.position.x, 0.5f + kEps);
        EXPECT_GE(v.position.y, -0.5f - kEps);
        EXPECT_LE(v.position.y, 0.5f + kEps);
        EXPECT_GE(v.position.z, -0.5f - kEps);
        EXPECT_LE(v.position.z, 0.5f + kEps);
    }
}

static void check_normals_unit(
    const std::vector<vw::FullVertex3D> &verts) {
    for (const auto &v : verts) {
        EXPECT_NEAR(glm::length(v.normal), 1.0f, kEps);
    }
}

static void check_tangents_orthogonal(
    const std::vector<vw::FullVertex3D> &verts) {
    for (const auto &v : verts) {
        EXPECT_NEAR(glm::dot(v.normal, v.tangeant), 0.0f, kEps);
        EXPECT_NEAR(glm::dot(v.normal, v.bitangeant), 0.0f,
                    kEps);
    }
}

static void check_uvs_in_range(
    const std::vector<vw::FullVertex3D> &verts) {
    for (const auto &v : verts) {
        EXPECT_GE(v.uv.x, -kEps);
        EXPECT_LE(v.uv.x, 1.0f + kEps);
        EXPECT_GE(v.uv.y, -kEps);
        EXPECT_LE(v.uv.y, 1.0f + kEps);
    }
}

// ── Triangle tests ──────────────────────────────────────────

TEST(PrimitiveTriangleTest, HasCorrectCounts) {
    auto result = create_triangle(PlanePrimitive::XY);
    EXPECT_EQ(result.vertices.size(), 3u);
    EXPECT_EQ(result.indices.size(), 3u);
}

TEST(PrimitiveTriangleTest, FitsInBoundingBox) {
    for (auto p : {PlanePrimitive::XY, PlanePrimitive::XZ,
                   PlanePrimitive::YZ}) {
        auto result = create_triangle(p);
        check_bounding_box(result.vertices);
    }
}

TEST(PrimitiveTriangleTest, NormalsAreUnitLength) {
    for (auto p : {PlanePrimitive::XY, PlanePrimitive::XZ,
                   PlanePrimitive::YZ}) {
        auto result = create_triangle(p);
        check_normals_unit(result.vertices);
    }
}

TEST(PrimitiveTriangleTest, TangentsOrthogonalToNormals) {
    for (auto p : {PlanePrimitive::XY, PlanePrimitive::XZ,
                   PlanePrimitive::YZ}) {
        auto result = create_triangle(p);
        check_tangents_orthogonal(result.vertices);
    }
}

TEST(PrimitiveTriangleTest, UVsInRange) {
    for (auto p : {PlanePrimitive::XY, PlanePrimitive::XZ,
                   PlanePrimitive::YZ}) {
        auto result = create_triangle(p);
        check_uvs_in_range(result.vertices);
    }
}

TEST(PrimitiveTriangleTest, NormalPointsAlongCorrectAxis) {
    {
        auto r = create_triangle(PlanePrimitive::XY);
        for (const auto &v : r.vertices) {
            EXPECT_NEAR(v.normal.x, 0.0f, kEps);
            EXPECT_NEAR(v.normal.y, 0.0f, kEps);
            EXPECT_NEAR(v.normal.z, 1.0f, kEps);
        }
    }
    {
        auto r = create_triangle(PlanePrimitive::XZ);
        for (const auto &v : r.vertices) {
            EXPECT_NEAR(v.normal.x, 0.0f, kEps);
            EXPECT_NEAR(v.normal.y, 1.0f, kEps);
            EXPECT_NEAR(v.normal.z, 0.0f, kEps);
        }
    }
    {
        auto r = create_triangle(PlanePrimitive::YZ);
        for (const auto &v : r.vertices) {
            EXPECT_NEAR(v.normal.x, 1.0f, kEps);
            EXPECT_NEAR(v.normal.y, 0.0f, kEps);
            EXPECT_NEAR(v.normal.z, 0.0f, kEps);
        }
    }
}

// ── Square tests ────────────────────────────────────────────

TEST(PrimitiveSquareTest, HasCorrectCounts) {
    auto result = create_square(PlanePrimitive::XY);
    EXPECT_EQ(result.vertices.size(), 4u);
    EXPECT_EQ(result.indices.size(), 6u);
}

TEST(PrimitiveSquareTest, FitsInBoundingBox) {
    for (auto p : {PlanePrimitive::XY, PlanePrimitive::XZ,
                   PlanePrimitive::YZ}) {
        auto result = create_square(p);
        check_bounding_box(result.vertices);
    }
}

TEST(PrimitiveSquareTest, NormalsAreUnitLength) {
    for (auto p : {PlanePrimitive::XY, PlanePrimitive::XZ,
                   PlanePrimitive::YZ}) {
        auto result = create_square(p);
        check_normals_unit(result.vertices);
    }
}

TEST(PrimitiveSquareTest, TangentsOrthogonalToNormals) {
    for (auto p : {PlanePrimitive::XY, PlanePrimitive::XZ,
                   PlanePrimitive::YZ}) {
        auto result = create_square(p);
        check_tangents_orthogonal(result.vertices);
    }
}

TEST(PrimitiveSquareTest, UVsCoverFullRange) {
    auto result = create_square(PlanePrimitive::XY);
    float min_u = 1.0f, max_u = 0.0f;
    float min_v = 1.0f, max_v = 0.0f;
    for (const auto &v : result.vertices) {
        min_u = std::min(min_u, v.uv.x);
        max_u = std::max(max_u, v.uv.x);
        min_v = std::min(min_v, v.uv.y);
        max_v = std::max(max_v, v.uv.y);
    }
    EXPECT_NEAR(min_u, 0.0f, kEps);
    EXPECT_NEAR(max_u, 1.0f, kEps);
    EXPECT_NEAR(min_v, 0.0f, kEps);
    EXPECT_NEAR(max_v, 1.0f, kEps);
}

TEST(PrimitiveSquareTest, AllPlanes) {
    for (auto p : {PlanePrimitive::XY, PlanePrimitive::XZ,
                   PlanePrimitive::YZ}) {
        auto result = create_square(p);
        EXPECT_EQ(result.vertices.size(), 4u);
        EXPECT_EQ(result.indices.size(), 6u);
        check_uvs_in_range(result.vertices);
    }
}

// ── Cube tests ──────────────────────────────────────────────

TEST(PrimitiveCubeTest, HasCorrectCounts) {
    auto result = create_cube();
    EXPECT_EQ(result.vertices.size(), 24u);
    EXPECT_EQ(result.indices.size(), 36u);
}

TEST(PrimitiveCubeTest, FitsInBoundingBox) {
    auto result = create_cube();
    check_bounding_box(result.vertices);
}

TEST(PrimitiveCubeTest, NormalsAreUnitLength) {
    auto result = create_cube();
    check_normals_unit(result.vertices);
}

TEST(PrimitiveCubeTest, TangentsOrthogonalToNormals) {
    auto result = create_cube();
    check_tangents_orthogonal(result.vertices);
}

TEST(PrimitiveCubeTest, EachFaceHasConsistentNormal) {
    auto result = create_cube();
    // Each group of 4 vertices (one face) should share the
    // same normal.
    for (size_t face = 0; face < 6; ++face) {
        auto base = face * 4;
        auto n0 = result.vertices[base].normal;
        for (size_t i = 1; i < 4; ++i) {
            auto ni = result.vertices[base + i].normal;
            EXPECT_NEAR(ni.x, n0.x, kEps);
            EXPECT_NEAR(ni.y, n0.y, kEps);
            EXPECT_NEAR(ni.z, n0.z, kEps);
        }
    }
}

TEST(PrimitiveCubeTest, AllSixNormalDirectionsCovered) {
    auto result = create_cube();

    // Collect unique normals (quantized to avoid float issues).
    auto key = [](const glm::vec3 &n) {
        return std::tuple{std::round(n.x), std::round(n.y),
                          std::round(n.z)};
    };

    std::set<std::tuple<float, float, float>> normals;
    for (size_t face = 0; face < 6; ++face) {
        normals.insert(key(result.vertices[face * 4].normal));
    }

    EXPECT_EQ(normals.size(), 6u);
    EXPECT_TRUE(normals.count({1, 0, 0}));
    EXPECT_TRUE(normals.count({-1, 0, 0}));
    EXPECT_TRUE(normals.count({0, 1, 0}));
    EXPECT_TRUE(normals.count({0, -1, 0}));
    EXPECT_TRUE(normals.count({0, 0, 1}));
    EXPECT_TRUE(normals.count({0, 0, -1}));
}

TEST(PrimitiveCubeTest, UVsInRange) {
    auto result = create_cube();
    check_uvs_in_range(result.vertices);
}
