#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Model/Mesh.h"
#include "VulkanWrapper/Model/Scene.h"
#include "VulkanWrapper/RayTracing/RayTracedScene.h"
#include "VulkanWrapper/RenderPass/ZPass.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

namespace vw::tests {

namespace {

std::filesystem::path get_shader_dir() {
    return std::filesystem::path(__FILE__)
               .parent_path()
               .parent_path()
               .parent_path() /
           "Shaders";
}

struct UBO {
    glm::mat4 proj;
    glm::mat4 view;
};

} // anonymous namespace

class ZPassTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = create_gpu();
        device = gpu.device;
        allocator = gpu.allocator;
        queue = &gpu.queue();

        cmdPool = std::make_unique<CommandPool>(
            CommandPoolBuilder(device).build());
    }

    std::unique_ptr<ZPass> create_pass() {
        return std::make_unique<ZPass>(
            device, allocator, get_shader_dir());
    }

    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;
    Queue *queue;
    std::unique_ptr<CommandPool> cmdPool;
};

TEST_F(ZPassTest, OutputSlots_ContainsDepth) {
    auto pass = create_pass();
    auto slots = pass->output_slots();

    ASSERT_EQ(slots.size(), 1u);
    EXPECT_EQ(slots[0], Slot::Depth);
}

TEST_F(ZPassTest, InputSlots_Empty) {
    auto pass = create_pass();
    auto slots = pass->input_slots();

    EXPECT_TRUE(slots.empty());
}

TEST_F(ZPassTest, Name_ReturnsZPass) {
    auto pass = create_pass();
    EXPECT_EQ(pass->name(), "ZPass");
}

TEST_F(ZPassTest, Execute_ProducesDepthImage) {
    constexpr Width width{64};
    constexpr Height height{64};

    auto pass = create_pass();

    // Create UBO
    UBO ubo_data{};
    ubo_data.proj = glm::perspective(
        glm::radians(60.0f), 1.0f, 0.1f, 100.0f);
    ubo_data.view = glm::lookAt(
        glm::vec3(0, 0, 5), glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0));

    auto ubo = create_buffer<
        Buffer<UBO, true, UniformBufferUsage>>(
        *allocator, 1);
    ubo.write(ubo_data, 0);

    // Create a RayTracedScene with no geometry (empty scene)
    rt::RayTracedScene scene(device, allocator);

    pass->set_uniform_buffer(ubo);
    pass->set_scene(scene);

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    pass->execute(cmd, tracker, width, height, 0);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    // Check result images
    auto results = pass->result_images();
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].first, Slot::Depth);
    EXPECT_EQ(results[0].second.image->format(),
              vk::Format::eD32Sfloat);
}

TEST_F(ZPassTest, Execute_DepthDimensions) {
    constexpr Width width{128};
    constexpr Height height{64};

    auto pass = create_pass();

    UBO ubo_data{};
    ubo_data.proj = glm::perspective(
        glm::radians(60.0f), 2.0f, 0.1f, 100.0f);
    ubo_data.view = glm::lookAt(
        glm::vec3(0, 0, 5), glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0));

    auto ubo = create_buffer<
        Buffer<UBO, true, UniformBufferUsage>>(
        *allocator, 1);
    ubo.write(ubo_data, 0);

    rt::RayTracedScene scene(device, allocator);

    pass->set_uniform_buffer(ubo);
    pass->set_scene(scene);

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    pass->execute(cmd, tracker, width, height, 0);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    auto results = pass->result_images();
    ASSERT_EQ(results.size(), 1u);

    auto &depth_image = results[0].second.image;
    EXPECT_EQ(depth_image->extent2D().width,
              static_cast<uint32_t>(width));
    EXPECT_EQ(depth_image->extent2D().height,
              static_cast<uint32_t>(height));
}

TEST_F(ZPassTest, Execute_Resize_CreatesNewImage) {
    auto pass = create_pass();

    UBO ubo_data{};
    ubo_data.proj = glm::perspective(
        glm::radians(60.0f), 1.0f, 0.1f, 100.0f);
    ubo_data.view = glm::lookAt(
        glm::vec3(0, 0, 5), glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0));

    auto ubo = create_buffer<
        Buffer<UBO, true, UniformBufferUsage>>(
        *allocator, 1);
    ubo.write(ubo_data, 0);

    rt::RayTracedScene scene(device, allocator);

    pass->set_uniform_buffer(ubo);
    pass->set_scene(scene);

    // First execute at 64x64
    {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore =
            cmd.begin(vk::CommandBufferBeginInfo().setFlags(
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        pass->execute(cmd, tracker, Width{64}, Height{64}, 0);

        std::ignore = cmd.end();
        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();
    }

    auto results1 = pass->result_images();
    ASSERT_EQ(results1.size(), 1u);
    EXPECT_EQ(results1[0].second.image->extent2D().width, 64u);

    // Second execute at 128x128
    {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore =
            cmd.begin(vk::CommandBufferBeginInfo().setFlags(
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        pass->execute(cmd, tracker, Width{128}, Height{128}, 0);

        std::ignore = cmd.end();
        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();
    }

    auto results2 = pass->result_images();
    ASSERT_EQ(results2.size(), 1u);
    EXPECT_EQ(results2[0].second.image->extent2D().width, 128u);
}

} // namespace vw::tests
