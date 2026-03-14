#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/RenderPass/AmbientOcclusionPass.h"
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

    Queue &queue() {
        return device->graphicsQueue();
    }
};

RayTracingGPU *create_ray_tracing_gpu() {
    try {
        auto instance =
            InstanceBuilder()
                .setDebug()
                .setApiVersion(ApiVersion::e13)
                .build();

        auto device =
            instance->findGpu()
                .with_queue(
                    vk::QueueFlagBits::eGraphics)
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
    static RayTracingGPU *gpu =
        create_ray_tracing_gpu();
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

class AmbientOcclusionPassTest
    : public ::testing::Test {
  protected:
    void SetUp() override {
        gpu = get_ray_tracing_gpu();
        if (!gpu) {
            GTEST_SKIP()
                << "Ray tracing not available";
        }
    }

    std::unique_ptr<AmbientOcclusionPass>
    create_pass() {
        // Use a null TLAS handle -- we only need it
        // for slot/name introspection tests, not for
        // actual rendering
        return std::make_unique<AmbientOcclusionPass>(
            gpu->device, gpu->allocator,
            get_shader_dir(),
            vk::AccelerationStructureKHR{});
    }

    RayTracingGPU *gpu = nullptr;
};

TEST_F(AmbientOcclusionPassTest,
       OutputSlots_ContainsAO) {
    auto pass = create_pass();
    auto slots = pass->output_slots();

    ASSERT_EQ(slots.size(), 1u);
    EXPECT_EQ(slots[0], Slot::AmbientOcclusion);
}

TEST_F(AmbientOcclusionPassTest,
       InputSlots_Contains5Slots) {
    auto pass = create_pass();
    auto slots = pass->input_slots();

    ASSERT_EQ(slots.size(), 5u);
    EXPECT_EQ(slots[0], Slot::Depth);
    EXPECT_EQ(slots[1], Slot::Position);
    EXPECT_EQ(slots[2], Slot::Normal);
    EXPECT_EQ(slots[3], Slot::Tangent);
    EXPECT_EQ(slots[4], Slot::Bitangent);
}

TEST_F(AmbientOcclusionPassTest,
       Name_ReturnsAmbientOcclusionPass) {
    auto pass = create_pass();
    EXPECT_EQ(pass->name(), "AmbientOcclusionPass");
}

} // namespace vw::tests
