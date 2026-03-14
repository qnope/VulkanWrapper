#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Model/Material/BindlessMaterialManager.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialHandler.h"
#include "VulkanWrapper/RayTracing/RayTracedScene.h"
#include "VulkanWrapper/RenderPass/DirectLightPass.h"
#include "VulkanWrapper/RenderPass/Slot.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include "VulkanWrapper/Vulkan/Instance.h"
#include <filesystem>
#include <gtest/gtest.h>

namespace vw::tests {

namespace {

struct RayTracingGPU {
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;

    Queue &queue() { return device->graphicsQueue(); }
};

RayTracingGPU *create_ray_tracing_gpu() {
    try {
        auto instance = InstanceBuilder()
                            .setDebug()
                            .setApiVersion(ApiVersion::e13)
                            .build();

        auto device =
            instance->findGpu()
                .with_queue(vk::QueueFlagBits::eGraphics)
                .with_synchronization_2()
                .with_dynamic_rendering()
                .with_ray_tracing()
                .with_descriptor_indexing()
                .build();

        auto allocator =
            AllocatorBuilder(instance, device).build();

        return new RayTracingGPU{std::move(instance),
                                 std::move(device),
                                 std::move(allocator)};
    } catch (...) {
        return nullptr;
    }
}

RayTracingGPU *get_ray_tracing_gpu() {
    static RayTracingGPU *gpu = create_ray_tracing_gpu();
    return gpu;
}

std::filesystem::path get_shader_dir() {
    return std::filesystem::path(__FILE__)
               .parent_path()
               .parent_path()
               .parent_path() /
           "Shaders";
}

} // anonymous namespace

class DirectLightPassTest : public ::testing::Test {
  protected:
    void SetUp() override {
        gpu = get_ray_tracing_gpu();
        if (!gpu) {
            GTEST_SKIP()
                << "Ray tracing not available";
        }
    }

    std::unique_ptr<DirectLightPass> create_pass() {
        auto staging =
            std::make_shared<StagingBufferManager>(
                gpu->device, gpu->allocator);
        m_material_manager =
            std::make_unique<
                Model::Material::BindlessMaterialManager>(
                gpu->device, gpu->allocator, staging);
        m_material_manager
            ->register_handler<
                Model::Material::ColoredMaterialHandler>();
        m_material_manager->upload_all();

        m_scene =
            std::make_unique<rt::RayTracedScene>(
                gpu->device, gpu->allocator);

        return std::make_unique<DirectLightPass>(
            gpu->device, gpu->allocator,
            get_shader_dir(), *m_scene,
            *m_material_manager);
    }

    RayTracingGPU *gpu = nullptr;
    std::unique_ptr<
        Model::Material::BindlessMaterialManager>
        m_material_manager;
    std::unique_ptr<rt::RayTracedScene> m_scene;
};

TEST_F(DirectLightPassTest,
       OutputSlots_Contains7Slots) {
    auto pass = create_pass();
    auto slots = pass->output_slots();

    ASSERT_EQ(slots.size(), 7u);
    EXPECT_EQ(slots[0], Slot::Albedo);
    EXPECT_EQ(slots[1], Slot::Normal);
    EXPECT_EQ(slots[2], Slot::Tangent);
    EXPECT_EQ(slots[3], Slot::Bitangent);
    EXPECT_EQ(slots[4], Slot::Position);
    EXPECT_EQ(slots[5], Slot::DirectLight);
    EXPECT_EQ(slots[6], Slot::IndirectRay);
}

TEST_F(DirectLightPassTest,
       InputSlots_ContainsDepth) {
    auto pass = create_pass();
    auto slots = pass->input_slots();

    ASSERT_EQ(slots.size(), 1u);
    EXPECT_EQ(slots[0], Slot::Depth);
}

TEST_F(DirectLightPassTest,
       Name_ReturnsDirectLightPass) {
    auto pass = create_pass();
    EXPECT_EQ(pass->name(), "DirectLightPass");
}

} // namespace vw::tests
