#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Model/Mesh.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/Pipeline/ComputePipeline.h"
#include "VulkanWrapper/Pipeline/PipelineLayout.h"
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
    EXPECT_EQ(sizeof(vw::rt::GeometryReference), 104);
}

TEST(GeometryReferenceStructTest, StructLayout) {
    vw::rt::GeometryReference ref{};
    ref.vertex_buffer_address = 0x123456789ABCDEF0ULL;
    ref.index_buffer_address = 0x0FEDCBA987654321ULL;
    ref.vertex_offset = 42;
    ref.first_index = 100;
    ref.material_type = 1;
    ref.material_address = 0xAAAABBBBCCCCDDDDULL;

    EXPECT_EQ(ref.vertex_buffer_address, 0x123456789ABCDEF0ULL);
    EXPECT_EQ(ref.index_buffer_address, 0x0FEDCBA987654321ULL);
    EXPECT_EQ(ref.vertex_offset, 42);
    EXPECT_EQ(ref.first_index, 100);
    EXPECT_EQ(ref.material_type, 1);
    EXPECT_EQ(ref.material_address, 0xAAAABBBBCCCCDDDDULL);
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
    EXPECT_EQ(ref.material_address, mesh.material().buffer_address);
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
#include "geometry_access.glsl"

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
    compiler.add_include_path("../../../VulkanWrapper/Shaders/include");

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

// =============================================================================
// Compute Shader Buffer Reference Execution Test
// =============================================================================

TEST_F(GeometryAccessTest, BufferReferenceComputeShaderExecution) {
    constexpr std::string_view compute_source = R"(
#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(local_size_x = 64) in;

layout(buffer_reference, scalar) buffer FloatBuffer { float values[]; };

layout(push_constant, scalar) uniform PushConstants {
    uint64_t input_address;
    uint64_t output_address;
    uint count;
};

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= count) return;
    FloatBuffer src = FloatBuffer(input_address);
    FloatBuffer dst = FloatBuffer(output_address);
    dst.values[idx] = src.values[idx] * 2.0;
}
)";

    constexpr uint32_t element_count = 256;

    // Compile compute shader
    vw::ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_3);
    auto shader_module = compiler.compile_to_module(
        gpu->device, compute_source, vk::ShaderStageFlagBits::eCompute);

    // Create pipeline layout with push constants
    struct PushConstants {
        uint64_t input_address;
        uint64_t output_address;
        uint32_t count;
    };

    auto pipeline_layout =
        vw::PipelineLayoutBuilder(gpu->device)
            .with_push_constant_range(
                vk::PushConstantRange()
                    .setStageFlags(vk::ShaderStageFlagBits::eCompute)
                    .setOffset(0)
                    .setSize(sizeof(PushConstants)))
            .build();

    // Build compute pipeline
    auto pipeline = vw::ComputePipelineBuilder(gpu->device,
                                               std::move(pipeline_layout))
                        .set_shader(shader_module)
                        .build();

    // Create input/output buffers
    auto input_buffer =
        vw::create_buffer<float, true, vw::StorageBufferUsage>(
            *gpu->allocator, element_count);
    auto output_buffer =
        vw::create_buffer<float, true, vw::StorageBufferUsage>(
            *gpu->allocator, element_count);

    // Fill input buffer
    std::vector<float> input_data(element_count);
    for (uint32_t i = 0; i < element_count; ++i) {
        input_data[i] = static_cast<float>(i) + 0.5f;
    }
    input_buffer.write(input_data, 0);

    // Zero output buffer
    std::vector<float> zeros(element_count, 0.0f);
    output_buffer.write(zeros, 0);

    // Record and submit command buffer
    auto cmd_pool = vw::CommandPoolBuilder(gpu->device).build();
    auto cmds = cmd_pool.allocate(1);
    auto cmd = cmds[0];

    std::ignore =
        cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->handle());

    PushConstants pc{
        .input_address = input_buffer.device_address(),
        .output_address = output_buffer.device_address(),
        .count = element_count,
    };
    cmd.pushConstants(pipeline->layout().handle(),
                      vk::ShaderStageFlagBits::eCompute, 0, sizeof(pc), &pc);

    cmd.dispatch(element_count / 64, 1, 1);

    std::ignore = cmd.end();

    gpu->queue().enqueue_command_buffer(cmd);
    gpu->queue().submit({}, {}, {}).wait();

    // Read back and verify
    auto result = output_buffer.read_as_vector(0, element_count);
    for (uint32_t i = 0; i < element_count; ++i) {
        EXPECT_FLOAT_EQ(result[i], input_data[i] * 2.0f)
            << "Mismatch at index " << i;
    }
}
