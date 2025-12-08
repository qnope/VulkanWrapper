#include "VulkanWrapper/Vulkan/Instance.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include <gtest/gtest.h>

// Instance Tests

TEST(InstanceTest, CreateInstance) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    EXPECT_NE(instance->handle(), nullptr);
}

TEST(InstanceTest, CreateInstanceWithoutDebug) {
    auto instance = vw::InstanceBuilder()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    EXPECT_NE(instance->handle(), nullptr);
}

TEST(InstanceTest, CreateInstanceWithPortability) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .addPortability()
                        .build();

    EXPECT_NE(instance->handle(), nullptr);
}

TEST(InstanceTest, FindGpu) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto deviceFinder = instance->findGpu();

    // Just verify that findGpu returns a DeviceFinder
    // We can't test much more without actually building a device
    SUCCEED();
}

// DeviceFinder Tests

TEST(DeviceFinderTest, BuildDeviceWithGraphicsQueue) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .build();

    EXPECT_NE(device->handle(), nullptr);
}

TEST(DeviceFinderTest, BuildDeviceWithSynchronization2) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .build();

    EXPECT_NE(device->handle(), nullptr);
}

TEST(DeviceFinderTest, BuildDeviceWithDynamicRendering) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_dynamic_rendering()
                      .build();

    EXPECT_NE(device->handle(), nullptr);
}

TEST(DeviceFinderTest, BuildDeviceWithAllCommonFeatures) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    EXPECT_NE(device->handle(), nullptr);
}

TEST(DeviceFinderTest, GetPhysicalDevice) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto physicalDevice = instance->findGpu()
                              .with_queue(vk::QueueFlagBits::eGraphics)
                              .get();

    EXPECT_TRUE(physicalDevice.has_value());
}

TEST(DeviceFinderTest, FluentApiChaining) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    // Test that fluent API chaining works
    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_queue(vk::QueueFlagBits::eCompute)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    EXPECT_NE(device->handle(), nullptr);
}

// Device Tests

TEST(DeviceTest, HandleAccessor) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    EXPECT_NE(device->handle(), nullptr);
}

TEST(DeviceTest, PhysicalDeviceAccessor) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    EXPECT_NE(device->physical_device(), nullptr);
}

TEST(DeviceTest, GraphicsQueueAccess) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    auto &queue = device->graphicsQueue();
    // If we get here without crashing, the queue is accessible
    SUCCEED();
}

TEST(DeviceTest, WaitIdle) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    // Should not throw
    device->wait_idle();
    SUCCEED();
}

TEST(DeviceTest, NonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<vw::Device>);
    EXPECT_FALSE(std::is_copy_assignable_v<vw::Device>);
}

TEST(DeviceTest, NonMovable) {
    EXPECT_FALSE(std::is_move_constructible_v<vw::Device>);
    EXPECT_FALSE(std::is_move_assignable_v<vw::Device>);
}

// Queue Tests

TEST(QueueTest, EnqueueAndSubmitSingleBuffer) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    auto pool = vw::CommandPoolBuilder(device).build();
    auto buffers = pool.allocate(1);

    // Record empty command buffer
    {
        vk::CommandBufferBeginInfo beginInfo{};
        buffers[0].begin(beginInfo);
        buffers[0].end();
    }

    auto &queue = device->graphicsQueue();
    queue.enqueue_command_buffer(buffers[0]);
    auto fence = queue.submit({}, {}, {});
    fence.wait();

    SUCCEED();
}

TEST(QueueTest, EnqueueAndSubmitMultipleBuffers) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    auto pool = vw::CommandPoolBuilder(device).build();
    auto buffers = pool.allocate(3);

    // Record empty command buffers
    for (auto &buffer : buffers) {
        vk::CommandBufferBeginInfo beginInfo{};
        buffer.begin(beginInfo);
        buffer.end();
    }

    auto &queue = device->graphicsQueue();
    queue.enqueue_command_buffers(buffers);
    auto fence = queue.submit({}, {}, {});
    fence.wait();

    SUCCEED();
}

TEST(QueueTest, MultipleSubmissions) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    auto pool = vw::CommandPoolBuilder(device).build();
    auto buffers = pool.allocate(2);

    // Record empty command buffers
    for (auto &buffer : buffers) {
        vk::CommandBufferBeginInfo beginInfo{};
        buffer.begin(beginInfo);
        buffer.end();
    }

    auto &queue = device->graphicsQueue();

    // First submission
    queue.enqueue_command_buffer(buffers[0]);
    auto fence1 = queue.submit({}, {}, {});
    fence1.wait();

    // Second submission
    queue.enqueue_command_buffer(buffers[1]);
    auto fence2 = queue.submit({}, {}, {});
    fence2.wait();

    SUCCEED();
}

TEST(QueueTest, SubmitEmptyQueue) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    auto &queue = device->graphicsQueue();
    // Submit with no command buffers enqueued
    auto fence = queue.submit({}, {}, {});
    fence.wait();

    SUCCEED();
}

TEST(QueueTest, CommandBuffersClearedAfterSubmit) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    auto pool = vw::CommandPoolBuilder(device).build();
    auto buffers = pool.allocate(1);

    vk::CommandBufferBeginInfo beginInfo{};
    buffers[0].begin(beginInfo);
    buffers[0].end();

    auto &queue = device->graphicsQueue();
    queue.enqueue_command_buffer(buffers[0]);

    // First submit
    auto fence1 = queue.submit({}, {}, {});
    fence1.wait();

    // Second submit should be empty (no crash, fence signaled immediately)
    auto fence2 = queue.submit({}, {}, {});
    fence2.wait();

    SUCCEED();
}
