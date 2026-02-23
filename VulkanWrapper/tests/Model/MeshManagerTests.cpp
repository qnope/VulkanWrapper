#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialHandler.h"
#include "VulkanWrapper/Model/Material/Material.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include <gtest/gtest.h>

using namespace vw::Model;

namespace {

struct MeshManagerGPU {
    std::shared_ptr<vw::Instance> instance;
    std::shared_ptr<vw::Device> device;
    std::shared_ptr<vw::Allocator> allocator;
};

MeshManagerGPU *create_mesh_manager_gpu() {
    try {
        auto instance = vw::InstanceBuilder()
                            .setDebug()
                            .setApiVersion(vw::ApiVersion::e13)
                            .build();

        auto device = instance->findGpu()
                          .with_queue(vk::QueueFlagBits::eGraphics)
                          .with_synchronization_2()
                          .with_dynamic_rendering()
                          .with_descriptor_indexing()
                          .with_ray_tracing()
                          .build();

        auto allocator = vw::AllocatorBuilder(instance, device).build();

        return new MeshManagerGPU{std::move(instance), std::move(device),
                                  std::move(allocator)};
    } catch (...) {
        return nullptr;
    }
}

MeshManagerGPU *get_gpu() {
    static MeshManagerGPU *gpu = create_mesh_manager_gpu();
    return gpu;
}

Material::Material make_dummy_material() {
    return {Material::colored_material_tag, 0};
}

std::vector<vw::FullVertex3D> make_triangle_vertices() {
    return {
        vw::FullVertex3D({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                         {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
        vw::FullVertex3D({1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                         {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
        vw::FullVertex3D({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                         {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
    };
}

std::vector<uint32_t> make_triangle_indices() { return {0, 1, 2}; }

std::vector<vw::FullVertex3D> make_quad_vertices() {
    return {
        vw::FullVertex3D({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                         {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
        vw::FullVertex3D({1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                         {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
        vw::FullVertex3D({1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                         {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
        vw::FullVertex3D({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                         {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
    };
}

std::vector<uint32_t> make_quad_indices() { return {0, 1, 2, 0, 2, 3}; }

} // namespace

class MeshManagerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        gpu = get_gpu();
        if (!gpu) {
            GTEST_SKIP() << "Ray tracing not available";
        }
        m_mesh_manager =
            std::make_unique<MeshManager>(gpu->device, gpu->allocator);
    }

    MeshManagerGPU *gpu = nullptr;
    std::unique_ptr<MeshManager> m_mesh_manager;
};

TEST_F(MeshManagerTest, InitiallyEmpty) {
    EXPECT_TRUE(m_mesh_manager->meshes().empty());
}

TEST_F(MeshManagerTest, AddSingleMeshIncreasesSize) {
    m_mesh_manager->add_mesh(make_triangle_vertices(),
                             make_triangle_indices(),
                             make_dummy_material());

    EXPECT_EQ(m_mesh_manager->meshes().size(), 1);
}

TEST_F(MeshManagerTest, AddedMeshHasCorrectIndexCount) {
    auto indices = make_triangle_indices();
    m_mesh_manager->add_mesh(make_triangle_vertices(), indices,
                             make_dummy_material());

    EXPECT_EQ(m_mesh_manager->meshes()[0].index_count(), indices.size());
}

TEST_F(MeshManagerTest, AddMultipleMeshes) {
    m_mesh_manager->add_mesh(make_triangle_vertices(),
                             make_triangle_indices(),
                             make_dummy_material());
    m_mesh_manager->add_mesh(make_quad_vertices(), make_quad_indices(),
                             make_dummy_material());

    EXPECT_EQ(m_mesh_manager->meshes().size(), 2);
}

TEST_F(MeshManagerTest, MultipleMeshesHaveCorrectIndexCounts) {
    m_mesh_manager->add_mesh(make_triangle_vertices(),
                             make_triangle_indices(),
                             make_dummy_material());
    m_mesh_manager->add_mesh(make_quad_vertices(), make_quad_indices(),
                             make_dummy_material());

    EXPECT_EQ(m_mesh_manager->meshes()[0].index_count(), 3);
    EXPECT_EQ(m_mesh_manager->meshes()[1].index_count(), 6);
}

TEST_F(MeshManagerTest, AddedMeshHasCorrectMaterialType) {
    m_mesh_manager->add_mesh(make_triangle_vertices(),
                             make_triangle_indices(),
                             make_dummy_material());

    EXPECT_EQ(m_mesh_manager->meshes()[0].material_type_tag(),
              Material::colored_material_tag);
}

TEST_F(MeshManagerTest, FillCommandBufferAfterAddMesh) {
    m_mesh_manager->add_mesh(make_triangle_vertices(),
                             make_triangle_indices(),
                             make_dummy_material());

    auto cmd = m_mesh_manager->fill_command_buffer();
    EXPECT_TRUE(cmd);
}

TEST_F(MeshManagerTest, FillCommandBufferWithMultipleMeshes) {
    m_mesh_manager->add_mesh(make_triangle_vertices(),
                             make_triangle_indices(),
                             make_dummy_material());
    m_mesh_manager->add_mesh(make_quad_vertices(), make_quad_indices(),
                             make_dummy_material());

    auto cmd = m_mesh_manager->fill_command_buffer();
    EXPECT_TRUE(cmd);
}
