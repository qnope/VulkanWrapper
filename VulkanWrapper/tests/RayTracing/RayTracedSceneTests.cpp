#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Model/Mesh.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/RayTracing/RayTracedScene.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>
#include <optional>
#include <set>

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
            // Use relative path from build/VulkanWrapper/tests/ to Models/
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

    const vw::Model::Mesh &get_mesh() { return get_cube_mesh(); }
};

std::optional<RayTracingGPU> create_ray_tracing_gpu() {
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
                          .build();

        auto allocator = vw::AllocatorBuilder(instance, device).build();

        return RayTracingGPU{std::move(instance), std::move(device),
                             std::move(allocator), std::nullopt};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<RayTracingGPU> &get_ray_tracing_gpu() {
    static std::optional<RayTracingGPU> gpu = create_ray_tracing_gpu();
    return gpu;
}

} // namespace

class RayTracedSceneTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu_opt = get_ray_tracing_gpu();
        if (!gpu_opt) {
            GTEST_SKIP() << "Ray tracing not available on this system";
        }
        gpu = &(*gpu_opt);
    }

    RayTracingGPU *gpu = nullptr;
};

TEST_F(RayTracedSceneTest, CreateEmptyScene) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    EXPECT_EQ(scene.mesh_count(), 0);
    EXPECT_EQ(scene.instance_count(), 0);
    EXPECT_EQ(scene.visible_instance_count(), 0);
    EXPECT_FALSE(scene.needs_build());
    EXPECT_FALSE(scene.needs_update());
}

TEST_F(RayTracedSceneTest, AddInstance) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto instance_id = scene.add_instance(mesh);

    EXPECT_EQ(instance_id.value, 0);
    EXPECT_EQ(scene.mesh_count(), 1);
    EXPECT_EQ(scene.instance_count(), 1);
    EXPECT_EQ(scene.visible_instance_count(), 1);
    EXPECT_TRUE(scene.needs_build());
}

TEST_F(RayTracedSceneTest, AddInstanceWithTransform) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    glm::mat4 transform =
        glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    auto instance_id = scene.add_instance(mesh, transform);

    auto retrieved_transform = scene.get_transform(instance_id);
    EXPECT_EQ(retrieved_transform, transform);
}

TEST_F(RayTracedSceneTest, AddMultipleInstancesOfSameMesh) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    [[maybe_unused]] auto inst1 = scene.add_instance(mesh);
    [[maybe_unused]] auto inst2 = scene.add_instance(
        mesh, glm::translate(glm::mat4(1.0f), glm::vec3(2, 0, 0)));
    [[maybe_unused]] auto inst3 = scene.add_instance(
        mesh, glm::translate(glm::mat4(1.0f), glm::vec3(4, 0, 0)));

    EXPECT_EQ(scene.mesh_count(), 1); // Same mesh geometry deduplicated
    EXPECT_EQ(scene.instance_count(), 3);
    EXPECT_EQ(scene.visible_instance_count(), 3);
}

TEST_F(RayTracedSceneTest, AddInstanceDeduplicates) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto inst1 = scene.add_instance(mesh, glm::mat4(1.0f));
    auto inst2 = scene.add_instance(
        mesh, glm::translate(glm::mat4(1.0f), glm::vec3(2, 0, 0)));

    // Same mesh should be deduplicated
    EXPECT_EQ(scene.mesh_count(), 1);
    EXPECT_EQ(scene.instance_count(), 2);
    EXPECT_NE(inst1, inst2);
}

TEST_F(RayTracedSceneTest, SetTransform) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    glm::mat4 new_transform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
    scene.set_transform(instance_id, new_transform);

    EXPECT_EQ(scene.get_transform(instance_id), new_transform);
}

TEST_F(RayTracedSceneTest, SetTransformWithInvalidId) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    EXPECT_THROW(scene.set_transform(vw::rt::InstanceId{999}, glm::mat4(1.0f)),
                 vw::LogicException);
}

TEST_F(RayTracedSceneTest, SetVisible) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    EXPECT_TRUE(scene.is_visible(instance_id));
    EXPECT_EQ(scene.visible_instance_count(), 1);

    scene.set_visible(instance_id, false);

    EXPECT_FALSE(scene.is_visible(instance_id));
    EXPECT_EQ(scene.visible_instance_count(), 0);
    EXPECT_EQ(scene.instance_count(), 1); // Still counted as active

    scene.set_visible(instance_id, true);

    EXPECT_TRUE(scene.is_visible(instance_id));
    EXPECT_EQ(scene.visible_instance_count(), 1);
}

TEST_F(RayTracedSceneTest, RemoveInstance) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    EXPECT_TRUE(scene.is_valid(instance_id));
    EXPECT_EQ(scene.instance_count(), 1);

    scene.remove_instance(instance_id);

    EXPECT_FALSE(scene.is_valid(instance_id));
    EXPECT_EQ(scene.instance_count(), 0);
    EXPECT_EQ(scene.visible_instance_count(), 0);
}

TEST_F(RayTracedSceneTest, RemoveInstanceTwice) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    scene.remove_instance(instance_id);

    EXPECT_THROW(scene.remove_instance(instance_id), vw::LogicException);
}

TEST_F(RayTracedSceneTest, OperationsOnRemovedInstance) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    scene.remove_instance(instance_id);

    EXPECT_THROW(scene.set_transform(instance_id, glm::mat4(1.0f)),
                 vw::LogicException);
    EXPECT_THROW((void)scene.get_transform(instance_id), vw::LogicException);
    EXPECT_THROW(scene.set_visible(instance_id, true), vw::LogicException);
    EXPECT_THROW((void)scene.is_visible(instance_id), vw::LogicException);
}

TEST_F(RayTracedSceneTest, SetSbtOffset) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    EXPECT_EQ(scene.get_sbt_offset(instance_id), 0);

    scene.set_sbt_offset(instance_id, 42);

    EXPECT_EQ(scene.get_sbt_offset(instance_id), 42);
}

TEST_F(RayTracedSceneTest, SetCustomIndex) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    EXPECT_EQ(scene.get_custom_index(instance_id), 0);

    scene.set_custom_index(instance_id, 123);

    EXPECT_EQ(scene.get_custom_index(instance_id), 123);
}

TEST_F(RayTracedSceneTest, DirtyFlags) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    EXPECT_FALSE(scene.needs_build());
    EXPECT_FALSE(scene.needs_update());

    [[maybe_unused]] auto instance_id = scene.add_instance(mesh);

    EXPECT_TRUE(scene.needs_build());
}

TEST_F(RayTracedSceneTest, BuildWithNoMeshes) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    EXPECT_THROW(scene.build(), vw::LogicException);
}

TEST_F(RayTracedSceneTest, UpdateBeforeBuild) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    [[maybe_unused]] auto inst = scene.add_instance(mesh);

    EXPECT_THROW(scene.update(), vw::LogicException);
}

TEST_F(RayTracedSceneTest, TlasAccessBeforeBuild) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    [[maybe_unused]] auto inst = scene.add_instance(mesh);

    EXPECT_THROW((void)scene.tlas_device_address(), vw::LogicException);
    EXPECT_THROW((void)scene.tlas_handle(), vw::LogicException);
}

TEST_F(RayTracedSceneTest, BuildAndAccess) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    [[maybe_unused]] auto inst = scene.add_instance(mesh);

    scene.build();

    EXPECT_FALSE(scene.needs_build());
    EXPECT_FALSE(scene.needs_update());
    EXPECT_NE(scene.tlas_device_address(), 0);
    EXPECT_NE(scene.tlas_handle(), vk::AccelerationStructureKHR{});
}

TEST_F(RayTracedSceneTest, BuildMultipleInstances) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    [[maybe_unused]] auto inst1 = scene.add_instance(mesh);
    [[maybe_unused]] auto inst2 = scene.add_instance(
        mesh, glm::translate(glm::mat4(1.0f), glm::vec3(2, 0, 0)));
    [[maybe_unused]] auto inst3 = scene.add_instance(
        mesh, glm::translate(glm::mat4(1.0f), glm::vec3(4, 0, 0)));

    scene.build();

    EXPECT_EQ(scene.instance_count(), 3);
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, UpdateAfterTransformChange) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    scene.build();

    [[maybe_unused]] auto old_address = scene.tlas_device_address();

    scene.set_transform(instance_id,
                        glm::translate(glm::mat4(1.0f), glm::vec3(10, 0, 0)));

    EXPECT_TRUE(scene.needs_update());

    scene.update();

    EXPECT_FALSE(scene.needs_update());
    // Address may or may not change, just verify TLAS is still valid
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, UpdateAfterVisibilityChange) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    scene.build();
    scene.set_visible(instance_id, false);

    EXPECT_TRUE(scene.needs_update());

    scene.update();

    EXPECT_FALSE(scene.needs_update());
}

TEST_F(RayTracedSceneTest, MoveScene) {
    vw::rt::RayTracedScene scene1(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    [[maybe_unused]] auto inst = scene1.add_instance(mesh);
    scene1.build();

    auto address1 = scene1.tlas_device_address();

    vw::rt::RayTracedScene scene2 = std::move(scene1);

    EXPECT_EQ(scene2.tlas_device_address(), address1);
    EXPECT_EQ(scene2.mesh_count(), 1);
    EXPECT_EQ(scene2.instance_count(), 1);
}

TEST_F(RayTracedSceneTest, InstanceIdEquality) {
    vw::rt::InstanceId id1{0};
    vw::rt::InstanceId id2{0};
    vw::rt::InstanceId id3{1};

    EXPECT_EQ(id1, id2);
    EXPECT_NE(id1, id3);
}

TEST_F(RayTracedSceneTest, AddInstancePopulatesScene) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(1, 2, 3));

    // Add instance using the simplified API
    std::ignore = scene.add_instance(mesh, transform);

    // Verify the embedded Scene is also populated
    const auto &embedded_scene = scene.scene();
    EXPECT_EQ(embedded_scene.size(), 1);
    EXPECT_EQ(embedded_scene.instances()[0].transform, transform);
}

TEST_F(RayTracedSceneTest, SceneAccessor) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    // Test const and non-const accessors
    const auto &const_scene = scene.scene();
    auto &mutable_scene = scene.scene();

    EXPECT_EQ(const_scene.size(), 0);
    EXPECT_EQ(mutable_scene.size(), 0);
}

TEST_F(RayTracedSceneTest, MeshGeometryHash) {
    const auto &mesh = gpu->get_mesh();

    // Same mesh should have same hash
    size_t hash1 = mesh.geometry_hash();
    size_t hash2 = mesh.geometry_hash();

    EXPECT_EQ(hash1, hash2);
    EXPECT_NE(hash1, 0); // Should not be zero
}

TEST_F(RayTracedSceneTest, MeshEquality) {
    const auto &mesh = gpu->get_mesh();

    // Same mesh should be equal to itself
    EXPECT_EQ(mesh, mesh);
}

TEST_F(RayTracedSceneTest, BuildWithAllInstancesHidden) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    scene.set_visible(instance_id, false);

    // Building with no visible instances should still work (empty TLAS)
    EXPECT_NO_THROW(scene.build());
    EXPECT_EQ(scene.visible_instance_count(), 0);
}

TEST_F(RayTracedSceneTest, AddInstanceAfterRemoval) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto inst1 = scene.add_instance(mesh);
    scene.remove_instance(inst1);

    auto inst2 = scene.add_instance(mesh);

    // New instance should have a different ID (IDs are not reused)
    EXPECT_NE(inst1, inst2);
    EXPECT_EQ(scene.instance_count(), 1); // Only one active instance
}

TEST_F(RayTracedSceneTest, BatchedOperationsBeforeUpdate) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto inst1 = scene.add_instance(mesh);
    auto inst2 = scene.add_instance(
        mesh, glm::translate(glm::mat4(1.0f), glm::vec3(2, 0, 0)));

    scene.build();

    // Batch multiple operations before calling update
    scene.set_transform(inst1, glm::scale(glm::mat4(1.0f), glm::vec3(2.0f)));
    scene.set_visible(inst2, false);
    scene.set_sbt_offset(inst1, 5);

    EXPECT_TRUE(scene.needs_update());

    scene.update();

    EXPECT_FALSE(scene.needs_update());
    EXPECT_EQ(scene.visible_instance_count(), 1);
}

TEST_F(RayTracedSceneTest, SbtOffsetOnRemovedInstance) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    scene.remove_instance(instance_id);

    EXPECT_THROW(scene.set_sbt_offset(instance_id, 1), vw::LogicException);
    EXPECT_THROW((void)scene.get_sbt_offset(instance_id), vw::LogicException);
}

TEST_F(RayTracedSceneTest, CustomIndexOnRemovedInstance) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    auto instance_id = scene.add_instance(mesh);

    scene.remove_instance(instance_id);

    EXPECT_THROW(scene.set_custom_index(instance_id, 1), vw::LogicException);
    EXPECT_THROW((void)scene.get_custom_index(instance_id), vw::LogicException);
}

TEST_F(RayTracedSceneTest, RebuildAfterAlreadyBuilt) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    [[maybe_unused]] auto inst = scene.add_instance(mesh);

    scene.build();

    [[maybe_unused]] auto address1 = scene.tlas_device_address();

    // Calling build again should work (rebuild from scratch)
    EXPECT_NO_THROW(scene.build());

    // TLAS should still be valid
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, ScenePopulatedWithMultipleDeduplicatedInstances) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    // Add multiple instances of the same mesh using simplified API
    std::ignore = scene.add_instance(mesh, glm::mat4(1.0f));
    std::ignore = scene.add_instance(
        mesh, glm::translate(glm::mat4(1.0f), glm::vec3(2, 0, 0)));
    std::ignore = scene.add_instance(
        mesh, glm::translate(glm::mat4(1.0f), glm::vec3(4, 0, 0)));

    // Embedded scene should have all 3 instances
    const auto &embedded_scene = scene.scene();
    EXPECT_EQ(embedded_scene.size(), 3);

    // But only one unique mesh geometry
    EXPECT_EQ(scene.mesh_count(), 1);
}

TEST_F(RayTracedSceneTest, UpdateWithNoChanges) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();
    [[maybe_unused]] auto inst = scene.add_instance(mesh);

    scene.build();

    EXPECT_FALSE(scene.needs_update());

    // Calling update when nothing changed should be fine
    EXPECT_NO_THROW(scene.update());
}

TEST_F(RayTracedSceneTest, VisibleInstanceCountAfterMixedOperations) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto inst1 = scene.add_instance(mesh);
    auto inst2 = scene.add_instance(mesh);
    auto inst3 = scene.add_instance(mesh);

    EXPECT_EQ(scene.visible_instance_count(), 3);

    scene.set_visible(inst1, false);
    EXPECT_EQ(scene.visible_instance_count(), 2);

    scene.remove_instance(inst2);
    EXPECT_EQ(scene.visible_instance_count(), 1);
    EXPECT_EQ(scene.instance_count(),
              2); // inst1 still active (just hidden), inst3 active

    scene.set_visible(inst1, true);
    EXPECT_EQ(scene.visible_instance_count(), 2);
}

// =============================================================================
// Multi-Mesh Tests - Testing with different mesh geometries
// =============================================================================

TEST_F(RayTracedSceneTest, MultipleDifferentMeshes) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    auto cube_inst = scene.add_instance(cube);
    auto plane_inst = scene.add_instance(plane);

    // Different meshes should create different BLAS entries
    EXPECT_EQ(scene.mesh_count(), 2);
    EXPECT_EQ(scene.instance_count(), 2);
    EXPECT_NE(cube_inst, plane_inst);
}

TEST_F(RayTracedSceneTest, MultipleDifferentMeshesWithDeduplication) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    // Add multiple instances of each mesh type
    [[maybe_unused]] auto c1 = scene.add_instance(cube, glm::mat4(1.0f));
    [[maybe_unused]] auto c2 = scene.add_instance(
        cube, glm::translate(glm::mat4(1.0f), glm::vec3(2, 0, 0)));
    [[maybe_unused]] auto p1 = scene.add_instance(plane, glm::mat4(1.0f));
    [[maybe_unused]] auto p2 = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1, 0)));
    [[maybe_unused]] auto c3 = scene.add_instance(
        cube, glm::translate(glm::mat4(1.0f), glm::vec3(4, 0, 0)));

    // Only 2 unique mesh geometries (cube and plane)
    EXPECT_EQ(scene.mesh_count(), 2);
    // But 5 total instances
    EXPECT_EQ(scene.instance_count(), 5);
    EXPECT_EQ(scene.visible_instance_count(), 5);
}

TEST_F(RayTracedSceneTest, BuildWithMultipleDifferentMeshes) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    [[maybe_unused]] auto c1 = scene.add_instance(cube);
    [[maybe_unused]] auto p1 = scene.add_instance(plane);

    scene.build();

    EXPECT_FALSE(scene.needs_build());
    EXPECT_NE(scene.tlas_device_address(), 0);
    EXPECT_EQ(scene.mesh_count(), 2);
}

TEST_F(RayTracedSceneTest, DifferentMeshesHaveDifferentHashes) {
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    // Different geometries should have different hashes
    EXPECT_NE(cube.geometry_hash(), plane.geometry_hash());
    EXPECT_NE(cube, plane);
}

// =============================================================================
// Stress Tests - Testing with many instances
// =============================================================================

TEST_F(RayTracedSceneTest, StressTestManyInstances) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    constexpr size_t instance_count = 100;
    std::vector<vw::rt::InstanceId> ids;
    ids.reserve(instance_count);

    for (size_t i = 0; i < instance_count; ++i) {
        float x = static_cast<float>(i % 10) * 2.0f;
        float y = static_cast<float>(i / 10) * 2.0f;
        auto transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
        ids.push_back(scene.add_instance(mesh, transform));
    }

    EXPECT_EQ(scene.mesh_count(), 1);
    EXPECT_EQ(scene.instance_count(), instance_count);
    EXPECT_EQ(scene.visible_instance_count(), instance_count);

    // All IDs should be unique
    std::set<uint32_t> unique_ids;
    for (const auto &id : ids) {
        unique_ids.insert(id.value);
    }
    EXPECT_EQ(unique_ids.size(), instance_count);

    scene.build();
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, StressTestManyInstancesWithMixedMeshes) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    constexpr size_t instances_per_mesh = 50;

    for (size_t i = 0; i < instances_per_mesh; ++i) {
        auto transform =
            glm::translate(glm::mat4(1.0f), glm::vec3(float(i) * 2, 0, 0));
        std::ignore = scene.add_instance(cube, transform);
    }

    for (size_t i = 0; i < instances_per_mesh; ++i) {
        auto transform =
            glm::translate(glm::mat4(1.0f), glm::vec3(float(i) * 2, 5, 0));
        std::ignore = scene.add_instance(plane, transform);
    }

    EXPECT_EQ(scene.mesh_count(), 2);
    EXPECT_EQ(scene.instance_count(), instances_per_mesh * 2);

    scene.build();
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, StressTestAddRemoveCycles) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    // Add and remove instances in cycles
    for (int cycle = 0; cycle < 5; ++cycle) {
        std::vector<vw::rt::InstanceId> ids;
        for (int i = 0; i < 20; ++i) {
            ids.push_back(scene.add_instance(mesh));
        }

        // Remove half of them
        for (size_t i = 0; i < ids.size(); i += 2) {
            scene.remove_instance(ids[i]);
        }
    }

    // Should have 10 instances from each of 5 cycles = 50 active instances
    EXPECT_EQ(scene.instance_count(), 50);
    EXPECT_EQ(scene.mesh_count(), 1);

    scene.build();
    EXPECT_NE(scene.tlas_device_address(), 0);
}

// =============================================================================
// Complex Transform Tests
// =============================================================================

TEST_F(RayTracedSceneTest, TransformWithRotation) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    glm::mat4 rotation =
        glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 1, 0));
    auto id = scene.add_instance(mesh, rotation);

    EXPECT_EQ(scene.get_transform(id), rotation);
}

TEST_F(RayTracedSceneTest, TransformWithNonUniformScale) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 0.5f, 1.0f));
    auto id = scene.add_instance(mesh, scale);

    EXPECT_EQ(scene.get_transform(id), scale);

    scene.build();
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, TransformWithNegativeScale) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    // Negative scale (mirrors the geometry)
    glm::mat4 mirror =
        glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    auto id = scene.add_instance(mesh, mirror);

    EXPECT_EQ(scene.get_transform(id), mirror);

    scene.build();
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, TransformWithCombinedTRS) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    // Combined Translation * Rotation * Scale
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(10.0f, 5.0f, -3.0f));
    transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(0, 0, 1));
    transform = glm::scale(transform, glm::vec3(2.0f));

    auto id = scene.add_instance(mesh, transform);

    EXPECT_EQ(scene.get_transform(id), transform);

    scene.build();
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, TransformWithVeryLargeValues) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    glm::mat4 large_transform = glm::translate(
        glm::mat4(1.0f), glm::vec3(10000.0f, 10000.0f, 10000.0f));
    auto id = scene.add_instance(mesh, large_transform);

    EXPECT_EQ(scene.get_transform(id), large_transform);

    scene.build();
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, TransformWithVerySmallScale) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    glm::mat4 tiny_scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.001f));
    auto id = scene.add_instance(mesh, tiny_scale);

    EXPECT_EQ(scene.get_transform(id), tiny_scale);

    scene.build();
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, UpdateManyTransformsBeforeSingleUpdate) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    std::vector<vw::rt::InstanceId> ids;
    for (int i = 0; i < 20; ++i) {
        ids.push_back(scene.add_instance(mesh));
    }

    scene.build();

    // Modify all transforms before calling update
    for (size_t i = 0; i < ids.size(); ++i) {
        auto new_transform = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(float(i) * 3.0f, sin(float(i)), cos(float(i))));
        scene.set_transform(ids[i], new_transform);
    }

    EXPECT_TRUE(scene.needs_update());

    scene.update();

    EXPECT_FALSE(scene.needs_update());
    EXPECT_NE(scene.tlas_device_address(), 0);
}

// =============================================================================
// Complex Operation Sequences
// =============================================================================

TEST_F(RayTracedSceneTest, AddInstanceAfterBuild) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    [[maybe_unused]] auto inst1 = scene.add_instance(mesh);
    scene.build();

    [[maybe_unused]] auto address_before = scene.tlas_device_address();

    // Adding instance of existing mesh only requires TLAS update (not full
    // rebuild)
    [[maybe_unused]] auto inst2 = scene.add_instance(mesh);
    EXPECT_FALSE(scene.needs_build()); // No new BLAS needed
    EXPECT_TRUE(scene.needs_update()); // TLAS needs update

    scene.update();
    EXPECT_NE(scene.tlas_device_address(), 0);
    EXPECT_EQ(scene.instance_count(), 2);
}

TEST_F(RayTracedSceneTest, RemoveInstanceAfterBuild) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto inst1 = scene.add_instance(mesh);
    [[maybe_unused]] auto inst2 = scene.add_instance(mesh);
    scene.build();

    scene.remove_instance(inst1);

    // Removal only requires TLAS update (BLAS unchanged)
    EXPECT_FALSE(scene.needs_build());
    EXPECT_TRUE(scene.needs_update());

    scene.update();
    EXPECT_EQ(scene.instance_count(), 1);
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, ComplexLifecycleSequence) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    // Phase 1: Add instances and build
    auto c1 = scene.add_instance(cube);
    auto c2 = scene.add_instance(
        cube, glm::translate(glm::mat4(1.0f), glm::vec3(2, 0, 0)));
    auto p1 = scene.add_instance(plane);

    EXPECT_EQ(scene.mesh_count(), 2);
    EXPECT_EQ(scene.instance_count(), 3);

    scene.build();
    auto addr1 = scene.tlas_device_address();
    EXPECT_NE(addr1, 0);

    // Phase 2: Update transforms
    scene.set_transform(c1, glm::scale(glm::mat4(1.0f), glm::vec3(2.0f)));
    scene.set_transform(p1,
                        glm::translate(glm::mat4(1.0f), glm::vec3(0, -2, 0)));

    EXPECT_TRUE(scene.needs_update());
    scene.update();
    EXPECT_FALSE(scene.needs_update());

    // Phase 3: Hide some instances
    scene.set_visible(c2, false);
    EXPECT_EQ(scene.visible_instance_count(), 2);
    scene.update();

    // Phase 4: Remove an instance and add new ones (using existing meshes)
    scene.remove_instance(c2);
    auto c3 = scene.add_instance(
        cube, glm::translate(glm::mat4(1.0f), glm::vec3(5, 0, 0)));
    auto c4 = scene.add_instance(
        cube, glm::translate(glm::mat4(1.0f), glm::vec3(7, 0, 0)));

    // Using existing meshes: only TLAS update needed (not full rebuild)
    EXPECT_FALSE(scene.needs_build());
    EXPECT_TRUE(scene.needs_update());
    scene.update();

    EXPECT_EQ(scene.mesh_count(), 2);
    EXPECT_EQ(scene.instance_count(), 4); // c1, p1, c3, c4
    EXPECT_TRUE(scene.is_valid(c1));
    EXPECT_TRUE(scene.is_valid(p1));
    EXPECT_TRUE(scene.is_valid(c3));
    EXPECT_TRUE(scene.is_valid(c4));
    EXPECT_FALSE(scene.is_valid(c2));

    // Phase 5: Final visibility changes
    scene.set_visible(c1, false);
    scene.set_visible(c3, false);
    EXPECT_EQ(scene.visible_instance_count(), 2); // p1 and c4

    scene.update();
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, ToggleVisibilityMultipleTimes) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto id = scene.add_instance(mesh);
    scene.build();

    for (int i = 0; i < 10; ++i) {
        scene.set_visible(id, false);
        EXPECT_FALSE(scene.is_visible(id));
        EXPECT_EQ(scene.visible_instance_count(), 0);

        scene.set_visible(id, true);
        EXPECT_TRUE(scene.is_visible(id));
        EXPECT_EQ(scene.visible_instance_count(), 1);
    }

    scene.update();
    EXPECT_NE(scene.tlas_device_address(), 0);
}

TEST_F(RayTracedSceneTest, AlternatingAddAndRemove) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    std::vector<vw::rt::InstanceId> active_ids;

    // Alternating add and remove
    for (int i = 0; i < 30; ++i) {
        auto id = scene.add_instance(mesh);
        active_ids.push_back(id);

        // Every 3rd addition, remove the oldest active instance
        if (i % 3 == 2 && !active_ids.empty()) {
            scene.remove_instance(active_ids.front());
            active_ids.erase(active_ids.begin());
        }
    }

    // Expected: 30 adds - 10 removes = 20 instances
    EXPECT_EQ(scene.instance_count(), 20);

    scene.build();
    EXPECT_NE(scene.tlas_device_address(), 0);
}

// =============================================================================
// Embedded Scene Synchronization Tests
// =============================================================================

TEST_F(RayTracedSceneTest, EmbeddedSceneSyncAfterRemoval) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto inst1 = scene.add_instance(mesh);
    [[maybe_unused]] auto inst2 = scene.add_instance(mesh);

    EXPECT_EQ(scene.scene().size(), 2);

    // Note: After removal, embedded scene may still have entries but ray
    // tracing structure won't include them. This tests that the counts are
    // consistent.
    scene.remove_instance(inst1);

    // instance_count reflects active ray tracing instances
    EXPECT_EQ(scene.instance_count(), 1);
}

TEST_F(RayTracedSceneTest, EmbeddedSceneTransformSync) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    glm::mat4 initial_transform =
        glm::translate(glm::mat4(1.0f), glm::vec3(5, 0, 0));
    [[maybe_unused]] auto id = scene.add_instance(mesh, initial_transform);

    const auto &embedded = scene.scene();
    EXPECT_EQ(embedded.instances()[0].transform, initial_transform);
}

TEST_F(RayTracedSceneTest, EmbeddedSceneWithMultipleMeshTypes) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    glm::mat4 cube_transform =
        glm::translate(glm::mat4(1.0f), glm::vec3(1, 0, 0));
    glm::mat4 plane_transform =
        glm::translate(glm::mat4(1.0f), glm::vec3(0, -1, 0));

    std::ignore = scene.add_instance(cube, cube_transform);
    std::ignore = scene.add_instance(plane, plane_transform);

    const auto &embedded = scene.scene();
    EXPECT_EQ(embedded.size(), 2);
    EXPECT_EQ(embedded.instances()[0].transform, cube_transform);
    EXPECT_EQ(embedded.instances()[1].transform, plane_transform);
}

// =============================================================================
// Edge Cases and Error Handling
// =============================================================================

TEST_F(RayTracedSceneTest, GetTransformWithInvalidId) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    EXPECT_THROW((void)scene.get_transform(vw::rt::InstanceId{999}),
                 vw::LogicException);
}

TEST_F(RayTracedSceneTest, IsValidWithInvalidId) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    // IDs beyond the allocated range should return false, not throw
    EXPECT_FALSE(scene.is_valid(vw::rt::InstanceId{999}));
}

TEST_F(RayTracedSceneTest, SetVisibleOnInvalidId) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    EXPECT_THROW(scene.set_visible(vw::rt::InstanceId{999}, true),
                 vw::LogicException);
}

TEST_F(RayTracedSceneTest, IsVisibleOnInvalidId) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    EXPECT_THROW((void)scene.is_visible(vw::rt::InstanceId{999}),
                 vw::LogicException);
}

TEST_F(RayTracedSceneTest, SetSbtOffsetOnInvalidId) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    EXPECT_THROW(scene.set_sbt_offset(vw::rt::InstanceId{999}, 0),
                 vw::LogicException);
}

TEST_F(RayTracedSceneTest, GetSbtOffsetOnInvalidId) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    EXPECT_THROW((void)scene.get_sbt_offset(vw::rt::InstanceId{999}),
                 vw::LogicException);
}

TEST_F(RayTracedSceneTest, SetCustomIndexOnInvalidId) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    EXPECT_THROW(scene.set_custom_index(vw::rt::InstanceId{999}, 0),
                 vw::LogicException);
}

TEST_F(RayTracedSceneTest, GetCustomIndexOnInvalidId) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    EXPECT_THROW((void)scene.get_custom_index(vw::rt::InstanceId{999}),
                 vw::LogicException);
}

TEST_F(RayTracedSceneTest, RemoveInvalidId) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);

    EXPECT_THROW(scene.remove_instance(vw::rt::InstanceId{999}),
                 vw::LogicException);
}

TEST_F(RayTracedSceneTest, DoubleSetSameTransform) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto id = scene.add_instance(mesh);
    scene.build();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(1, 2, 3));

    // Setting the same transform multiple times should be idempotent
    scene.set_transform(id, transform);
    scene.set_transform(id, transform);

    EXPECT_EQ(scene.get_transform(id), transform);
}

TEST_F(RayTracedSceneTest, SetVisibleToCurrentValue) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto id = scene.add_instance(mesh);

    // Instance starts visible
    EXPECT_TRUE(scene.is_visible(id));

    // Setting to current value should work without issues
    scene.set_visible(id, true);
    EXPECT_TRUE(scene.is_visible(id));
    EXPECT_EQ(scene.visible_instance_count(), 1);
}

// =============================================================================
// Build/Update State Machine Tests
// =============================================================================

TEST_F(RayTracedSceneTest, BuildSetsNeedsUpdateFalse) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto id = scene.add_instance(mesh);
    scene.set_transform(id, glm::scale(glm::mat4(1.0f), glm::vec3(2.0f)));

    // Before build, needs_build should be true
    EXPECT_TRUE(scene.needs_build());

    scene.build();

    // After build, both should be false
    EXPECT_FALSE(scene.needs_build());
    EXPECT_FALSE(scene.needs_update());
}

TEST_F(RayTracedSceneTest, TransformChangeAfterBuildSetsNeedsUpdate) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto id = scene.add_instance(mesh);
    scene.build();

    EXPECT_FALSE(scene.needs_update());

    scene.set_transform(id,
                        glm::translate(glm::mat4(1.0f), glm::vec3(5, 0, 0)));

    EXPECT_TRUE(scene.needs_update());
    EXPECT_FALSE(
        scene.needs_build()); // Transform change only needs update, not rebuild
}

TEST_F(RayTracedSceneTest, VisibilityChangeAfterBuildSetsNeedsUpdate) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto id = scene.add_instance(mesh);
    scene.build();

    EXPECT_FALSE(scene.needs_update());

    scene.set_visible(id, false);

    EXPECT_TRUE(scene.needs_update());
}

TEST_F(RayTracedSceneTest, SbtOffsetChangeAfterBuildSetsNeedsUpdate) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto id = scene.add_instance(mesh);
    scene.build();

    EXPECT_FALSE(scene.needs_update());

    scene.set_sbt_offset(id, 10);

    EXPECT_TRUE(scene.needs_update());
}

TEST_F(RayTracedSceneTest, CustomIndexChangeAfterBuildSetsNeedsUpdate) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto id = scene.add_instance(mesh);
    scene.build();

    EXPECT_FALSE(scene.needs_update());

    scene.set_custom_index(id, 42);

    EXPECT_TRUE(scene.needs_update());
}

TEST_F(RayTracedSceneTest, AddInstanceOfExistingMeshSetsNeedsUpdate) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    [[maybe_unused]] auto inst1 = scene.add_instance(mesh);
    scene.build();

    EXPECT_FALSE(scene.needs_build());
    EXPECT_FALSE(scene.needs_update());

    // Adding instance of EXISTING mesh only needs TLAS update
    [[maybe_unused]] auto inst2 = scene.add_instance(mesh);

    EXPECT_FALSE(scene.needs_build()); // No new BLAS needed
    EXPECT_TRUE(scene.needs_update()); // TLAS needs update
}

TEST_F(RayTracedSceneTest, RemoveInstanceSetsNeedsUpdate) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto inst1 = scene.add_instance(mesh);
    [[maybe_unused]] auto inst2 = scene.add_instance(mesh);
    scene.build();

    EXPECT_FALSE(scene.needs_build());
    EXPECT_FALSE(scene.needs_update());

    scene.remove_instance(inst1);

    // Removal only affects TLAS, not BLAS
    EXPECT_FALSE(scene.needs_build());
    EXPECT_TRUE(scene.needs_update());
}

TEST_F(RayTracedSceneTest, AddInstanceOfNewMeshRequiresBuild) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    // Build with only cube
    [[maybe_unused]] auto inst1 = scene.add_instance(cube);
    scene.build();

    EXPECT_FALSE(scene.needs_build());
    EXPECT_FALSE(scene.needs_update());
    EXPECT_EQ(scene.mesh_count(), 1);

    // Add instance of NEW mesh (plane) - this requires full rebuild
    [[maybe_unused]] auto inst2 = scene.add_instance(plane);

    EXPECT_TRUE(scene.needs_build());   // New BLAS needed for plane
    EXPECT_FALSE(scene.needs_update()); // needs_update returns false when
                                        // needs_build is true
    EXPECT_EQ(scene.mesh_count(), 2);

    // Must call build(), not update()
    scene.build();
    EXPECT_FALSE(scene.needs_build());
    EXPECT_EQ(scene.instance_count(), 2);
}

// =============================================================================
// Move Semantics Tests
// =============================================================================

TEST_F(RayTracedSceneTest, MoveAssignment) {
    vw::rt::RayTracedScene scene1(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    [[maybe_unused]] auto inst = scene1.add_instance(mesh);
    scene1.build();

    auto address1 = scene1.tlas_device_address();
    auto mesh_count1 = scene1.mesh_count();
    auto instance_count1 = scene1.instance_count();

    vw::rt::RayTracedScene scene2(gpu->device, gpu->allocator);
    scene2 = std::move(scene1);

    EXPECT_EQ(scene2.tlas_device_address(), address1);
    EXPECT_EQ(scene2.mesh_count(), mesh_count1);
    EXPECT_EQ(scene2.instance_count(), instance_count1);
}

TEST_F(RayTracedSceneTest, MoveSceneWithMultipleMeshes) {
    vw::rt::RayTracedScene scene1(gpu->device, gpu->allocator);
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    [[maybe_unused]] auto c1 = scene1.add_instance(cube);
    [[maybe_unused]] auto p1 = scene1.add_instance(plane);
    scene1.build();

    auto address = scene1.tlas_device_address();

    vw::rt::RayTracedScene scene2 = std::move(scene1);

    EXPECT_EQ(scene2.tlas_device_address(), address);
    EXPECT_EQ(scene2.mesh_count(), 2);
    EXPECT_EQ(scene2.instance_count(), 2);
}

// =============================================================================
// Consistency Tests
// =============================================================================

TEST_F(RayTracedSceneTest, InstanceIdsAreMonotonicallyIncreasing) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto id1 = scene.add_instance(mesh);
    auto id2 = scene.add_instance(mesh);
    auto id3 = scene.add_instance(mesh);

    EXPECT_LT(id1.value, id2.value);
    EXPECT_LT(id2.value, id3.value);
}

TEST_F(RayTracedSceneTest, InstanceIdsNotReusedAfterRemoval) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto id1 = scene.add_instance(mesh);
    auto id2 = scene.add_instance(mesh);
    uint32_t id2_value = id2.value;

    scene.remove_instance(id1);

    auto id3 = scene.add_instance(mesh);

    // New ID should be greater than all previous IDs, not reusing removed slot
    EXPECT_GT(id3.value, id2_value);
}

TEST_F(RayTracedSceneTest, MeshCountCorrectAfterMixedOperations) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &cube = gpu->get_cube_mesh();
    const auto &plane = gpu->get_plane_mesh();

    // Add only cubes
    auto c1 = scene.add_instance(cube);
    [[maybe_unused]] auto c2 = scene.add_instance(cube);
    EXPECT_EQ(scene.mesh_count(), 1);

    // Add plane
    [[maybe_unused]] auto p1 = scene.add_instance(plane);
    EXPECT_EQ(scene.mesh_count(), 2);

    // Remove one cube - mesh count shouldn't change as c2 still uses it
    scene.remove_instance(c1);
    EXPECT_EQ(scene.mesh_count(), 2);

    // Mesh count should still be 2 (BLAS entries are preserved for potential
    // reuse)
}

TEST_F(RayTracedSceneTest, AllHiddenThenOneRevealed) {
    vw::rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &mesh = gpu->get_mesh();

    auto id1 = scene.add_instance(mesh);
    auto id2 = scene.add_instance(mesh);
    auto id3 = scene.add_instance(mesh);

    scene.build();

    // Hide all
    scene.set_visible(id1, false);
    scene.set_visible(id2, false);
    scene.set_visible(id3, false);
    EXPECT_EQ(scene.visible_instance_count(), 0);

    scene.update();

    // Reveal one
    scene.set_visible(id2, true);
    EXPECT_EQ(scene.visible_instance_count(), 1);

    scene.update();
    EXPECT_NE(scene.tlas_device_address(), 0);
}
