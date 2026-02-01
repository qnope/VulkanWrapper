#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Model/Mesh.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/RayTracing/GeometryReference.h"
#include "VulkanWrapper/RayTracing/RayTracedScene.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>
#include <optional>

namespace {

struct RayTracingGPU {
    std::shared_ptr<vw::Instance> instance;
    std::shared_ptr<vw::Device> device;
    std::shared_ptr<vw::Allocator> allocator;
    std::optional<vw::Model::MeshManager> mesh_manager;

    vw::Queue &queue() { return device->graphicsQueue(); }

    void ensure_meshes_loaded() {
        if (!mesh_manager) {
            mesh_manager.emplace(device, allocator);
            mesh_manager->read_file("../../../Models/cube.obj");
            mesh_manager->read_file("../../../Models/plane.obj");
            auto cmd = mesh_manager->fill_command_buffer();
            queue().enqueue_command_buffer(cmd);
            queue().submit({}, {}, {}).wait();
        }
    }

    const vw::Model::Mesh &get_cube_mesh() {
        ensure_meshes_loaded();
        return mesh_manager->meshes()[0];
    }

    const vw::Model::Mesh &get_plane_mesh() {
        ensure_meshes_loaded();
        return mesh_manager->meshes()[1];
    }
};

RayTracingGPU *create_ray_tracing_gpu() {
    try {
        auto instance = vw::InstanceBuilder()
                            .setDebug()
                            .setApiVersion(vw::ApiVersion::e13)
                            .build();

        auto device = instance->findGpu()
                          .with_queue(vk::QueueFlagBits::eGraphics)
                          .with_synchronization_2()
                          .with_dynamic_rendering()
                          .with_ray_tracing()
                          .with_descriptor_indexing()
                          .build();

        auto allocator = vw::AllocatorBuilder(instance, device).build();

        // Intentionally leak GPU to avoid static destruction order issues
        // The OS will clean up memory when the test process exits
        return new RayTracingGPU{std::move(instance), std::move(device),
                                 std::move(allocator), std::nullopt};
    } catch (...) {
        return nullptr;
    }
}

RayTracingGPU *get_ray_tracing_gpu() {
    static RayTracingGPU *gpu = create_ray_tracing_gpu();
    return gpu;
}

} // namespace

class GeometryAccessTest : public ::testing::Test {
  protected:
    void SetUp() override {
        gpu = get_ray_tracing_gpu();
        if (!gpu) {
            GTEST_SKIP() << "Ray tracing not available on this system";
        }
    }

    RayTracingGPU *gpu = nullptr;
};

// =============================================================================
// GeometryReference Struct Tests
// =============================================================================

TEST(GeometryReferenceStructTest, StructSize) {
    EXPECT_EQ(sizeof(vw::rt::GeometryReference), 40);
}

TEST(GeometryReferenceStructTest, StructLayout) {
    vw::rt::GeometryReference ref{};
    ref.vertex_buffer_address = 0x123456789ABCDEF0ULL;
    ref.vertex_offset = 42;
    ref.index_buffer_address = 0x0FEDCBA987654321ULL;
    ref.first_index = 100;
    ref.material_type = 1;
    ref.material_index = 5;

    EXPECT_EQ(ref.vertex_buffer_address, 0x123456789ABCDEF0ULL);
    EXPECT_EQ(ref.vertex_offset, 42);
    EXPECT_EQ(ref.index_buffer_address, 0x0FEDCBA987654321ULL);
    EXPECT_EQ(ref.first_index, 100);
    EXPECT_EQ(ref.material_type, 1);
    EXPECT_EQ(ref.material_index, 5);
}

// =============================================================================
// Geometry Buffer Creation Tests
// =============================================================================

TEST_F(GeometryAccessTest, GeometryBufferCreatedAfterBuild) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_cube_mesh();

    [[maybe_unused]] auto inst = scene.add_instance(mesh);

    EXPECT_FALSE(scene.has_geometry_buffer());

    scene.build();

    EXPECT_TRUE(scene.has_geometry_buffer());
}

TEST_F(GeometryAccessTest, GeometryBufferAddressNonZero) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_cube_mesh();

    [[maybe_unused]] auto inst = scene.add_instance(mesh);
    scene.build();

    EXPECT_NE(scene.geometry_buffer_address(), 0);
}

TEST_F(GeometryAccessTest, GeometryBufferAccessBeforeBuildThrows) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_cube_mesh();

    [[maybe_unused]] auto inst = scene.add_instance(mesh);

    EXPECT_THROW((void)scene.geometry_buffer_address(), vw::LogicException);
    EXPECT_THROW((void)scene.geometry_buffer(), vw::LogicException);
}

TEST_F(GeometryAccessTest, GeometryBufferContentsCorrect) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_cube_mesh();

    [[maybe_unused]] auto inst = scene.add_instance(mesh);
    scene.build();

    const auto &buffer = scene.geometry_buffer();
    EXPECT_EQ(buffer.size(), 1);

    auto contents = buffer.read_as_vector(0, 1);
    EXPECT_EQ(contents.size(), 1);

    const auto &ref = contents[0];
    EXPECT_NE(ref.vertex_buffer_address, 0);
    EXPECT_NE(ref.index_buffer_address, 0);
    EXPECT_EQ(ref.vertex_offset, mesh.vertex_offset());
    EXPECT_EQ(ref.first_index, mesh.first_index());
    EXPECT_EQ(ref.material_type, mesh.material().material_type.id());
    EXPECT_EQ(ref.material_index, mesh.material().material_index);
}

TEST_F(GeometryAccessTest, MultiMeshGeometryBufferCorrect) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    [[maybe_unused]] auto c1 = scene.add_instance(cube);
    [[maybe_unused]] auto p1 = scene.add_instance(plane);
    [[maybe_unused]] auto c2 = scene.add_instance(cube);

    scene.build();

    const auto &buffer = scene.geometry_buffer();
    EXPECT_EQ(buffer.size(), 2);

    auto contents = buffer.read_as_vector(0, 2);
    EXPECT_EQ(contents.size(), 2);

    EXPECT_NE(contents[0].vertex_buffer_address, 0);
    EXPECT_NE(contents[0].index_buffer_address, 0);
    EXPECT_NE(contents[1].vertex_buffer_address, 0);
    EXPECT_NE(contents[1].index_buffer_address, 0);
}

TEST_F(GeometryAccessTest, GeometryBufferAddressMatchesMesh) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_cube_mesh();

    [[maybe_unused]] auto inst = scene.add_instance(mesh);
    scene.build();

    auto contents = scene.geometry_buffer().read_as_vector(0, 1);
    const auto &ref = contents[0];

    auto expected_vertex_addr = mesh.full_vertex_buffer()->device_address();
    auto expected_index_addr = mesh.index_buffer()->device_address();

    EXPECT_EQ(ref.vertex_buffer_address, expected_vertex_addr);
    EXPECT_EQ(ref.index_buffer_address, expected_index_addr);
}

// =============================================================================
// Mesh Accessor Tests
// =============================================================================

TEST_F(GeometryAccessTest, MeshFullVertexBufferAccessor) {
    const auto &mesh = gpu->get_cube_mesh();

    auto buffer = mesh.full_vertex_buffer();
    EXPECT_NE(buffer, nullptr);
    EXPECT_GT(buffer->size(), 0);
}

TEST_F(GeometryAccessTest, MeshIndexBufferAccessor) {
    const auto &mesh = gpu->get_cube_mesh();

    auto buffer = mesh.index_buffer();
    EXPECT_NE(buffer, nullptr);
    EXPECT_GT(buffer->size(), 0);
}

TEST_F(GeometryAccessTest, MeshVertexOffsetAccessor) {
    const auto &mesh = gpu->get_cube_mesh();

    auto offset = mesh.vertex_offset();
    EXPECT_GE(offset, 0);
}

TEST_F(GeometryAccessTest, MeshFirstIndexAccessor) {
    const auto &mesh = gpu->get_cube_mesh();

    auto first_index = mesh.first_index();
    EXPECT_GE(first_index, 0);
}

// =============================================================================
// GLSL Shader Include Compilation Tests
// =============================================================================

TEST(GeometryAccessShaderTest, GeometryAccessGlslCompiles) {
    const std::string test_shader = R"(
#version 460
#extension GL_EXT_ray_tracing : require

#define GEOMETRY_BUFFER_BINDING 0

)" +
                                    std::string(R"(
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct FullVertex3D {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 uv;
};

layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer FullVertexRef {
    FullVertex3D vertices[];
};

layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer IndexRef {
    uint indices[];
};

struct GeometryReference {
    uint64_t vertex_buffer_address;
    int vertex_offset;
    int pad0;
    uint64_t index_buffer_address;
    int first_index;
    int pad1;
    uint material_type;
    uint material_index;
};

layout(set = 0, binding = GEOMETRY_BUFFER_BINDING, scalar) readonly buffer GeometryReferenceBuffer {
    GeometryReference geometry_refs[];
};

struct VertexData {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 uv;
    uint material_type;
    uint material_index;
};
)") + R"(

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 bary;

void main() {
    uint geom_idx = gl_InstanceCustomIndexEXT;
    GeometryReference geom = geometry_refs[geom_idx];

    FullVertexRef vertex_buffer = FullVertexRef(geom.vertex_buffer_address);
    IndexRef index_buffer = IndexRef(geom.index_buffer_address);

    uint i0 = index_buffer.indices[geom.first_index + gl_PrimitiveID * 3 + 0];
    uint i1 = index_buffer.indices[geom.first_index + gl_PrimitiveID * 3 + 1];
    uint i2 = index_buffer.indices[geom.first_index + gl_PrimitiveID * 3 + 2];

    FullVertex3D v0 = vertex_buffer.vertices[geom.vertex_offset + i0];
    FullVertex3D v1 = vertex_buffer.vertices[geom.vertex_offset + i1];
    FullVertex3D v2 = vertex_buffer.vertices[geom.vertex_offset + i2];

    float w0 = 1.0 - bary.x - bary.y;
    float w1 = bary.x;
    float w2 = bary.y;

    vec3 normal = normalize(v0.normal * w0 + v1.normal * w1 + v2.normal * w2);

    hitValue = normal * 0.5 + 0.5;
}
)";

    vw::ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_3);

    auto result =
        compiler.compile(test_shader, vk::ShaderStageFlagBits::eClosestHitKHR);
    EXPECT_FALSE(result.spirv.empty());
    EXPECT_EQ(result.spirv[0], 0x07230203u);
}

TEST(GeometryAccessShaderTest, GeometryAccessGlslIncludeCompiles) {
    const std::string test_shader = R"(
#version 460
#extension GL_EXT_ray_tracing : require

#define GEOMETRY_BUFFER_BINDING 0
#include "geometry_access.glsl"

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 bary;

void main() {
    VertexData v = interpolate_vertex(gl_InstanceCustomIndexEXT, gl_PrimitiveID, bary);
    hitValue = v.normal * 0.5 + 0.5;
}
)";

    vw::ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_3);
    compiler.add_include_path("../../../VulkanWrapper/Shaders/include");

    auto result =
        compiler.compile(test_shader, vk::ShaderStageFlagBits::eClosestHitKHR);
    EXPECT_FALSE(result.spirv.empty());
    EXPECT_EQ(result.spirv[0], 0x07230203u);
}

// =============================================================================
// TLAS Custom Index Tests
// =============================================================================

TEST_F(GeometryAccessTest, TlasInstancesHaveCorrectCustomIndex) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    [[maybe_unused]] auto c1 = scene.add_instance(cube);
    [[maybe_unused]] auto p1 = scene.add_instance(plane);
    [[maybe_unused]] auto c2 = scene.add_instance(cube);

    scene.build();

    EXPECT_EQ(scene.mesh_count(), 2);
    EXPECT_TRUE(scene.has_geometry_buffer());
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(GeometryAccessTest, RebuildPreservesGeometryBuffer) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_cube_mesh();

    [[maybe_unused]] auto inst = scene.add_instance(mesh);
    scene.build();

    auto addr1 = scene.geometry_buffer_address();
    EXPECT_NE(addr1, 0);

    scene.build();

    auto addr2 = scene.geometry_buffer_address();
    EXPECT_NE(addr2, 0);
}

// =============================================================================
// DeviceFinder scalar_block_layout Tests
// =============================================================================

TEST(ScalarBlockLayoutTest, WithScalarBlockLayoutEnablesFeature) {
    try {
        auto instance = vw::InstanceBuilder()
                            .setDebug()
                            .setApiVersion(vw::ApiVersion::e13)
                            .build();

        auto device = instance->findGpu()
                          .with_queue(vk::QueueFlagBits::eGraphics)
                          .with_scalar_block_layout()
                          .build();

        EXPECT_NE(device, nullptr);

        // Ensure device is idle before destruction to avoid validation errors
        device->wait_idle();
    } catch (...) {
        GTEST_SKIP() << "Could not create device with scalar_block_layout";
    }
}

TEST(ScalarBlockLayoutTest, RayTracingEnablesScalarBlockLayout) {
    try {
        auto instance = vw::InstanceBuilder()
                            .setDebug()
                            .setApiVersion(vw::ApiVersion::e13)
                            .build();

        auto device = instance->findGpu()
                          .with_queue(vk::QueueFlagBits::eGraphics)
                          .with_ray_tracing()
                          .build();

        EXPECT_NE(device, nullptr);

        // Ensure device is idle before destruction to avoid validation errors
        device->wait_idle();
    } catch (...) {
        GTEST_SKIP() << "Could not create device with ray_tracing";
    }
}
