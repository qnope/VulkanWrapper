#include "VulkanWrapper/Vulkan/Instance.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include "VulkanWrapper/Utils/Error.h"
#include <gtest/gtest.h>

TEST(InstanceTest, CreateInstance) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
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

TEST(InstanceTest, ValidationLayerException_ThrownImmediately) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .addPortability()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    // Try to create a buffer with size 0, which should throw immediately
    vk::BufferCreateInfo invalidBufferInfo;
    invalidBufferInfo.setSize(0)  // Invalid: size must be > 0
        .setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
        .setSharingMode(vk::SharingMode::eExclusive);

    // The exception should be thrown during the Vulkan call itself
    EXPECT_THROW(
        (void)device->handle().createBuffer(invalidBufferInfo),
        vw::ValidationLayerException
    );
}

TEST(InstanceTest, ValidationLayerException_ContainsCorrectInfo) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .addPortability()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    // Trigger a validation error and catch the exception
    vk::BufferCreateInfo invalidBufferInfo;
    invalidBufferInfo.setSize(0)
        .setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
        .setSharingMode(vk::SharingMode::eExclusive);

    try {
        (void)device->handle().createBuffer(invalidBufferInfo);
        FAIL() << "Expected ValidationLayerException";
    } catch (const vw::ValidationLayerException &e) {
        // Verify the exception contains useful information
        EXPECT_EQ(e.severity(), vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        EXPECT_TRUE(e.type() & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        EXPECT_FALSE(e.validation_message().empty());

        // The what() message should contain the validation message
        std::string whatMsg = e.what();
        EXPECT_FALSE(whatMsg.empty());
        EXPECT_NE(whatMsg.find("VALIDATION"), std::string::npos);

        // Should mention the buffer size issue
        EXPECT_NE(e.validation_message().find("size"), std::string::npos);
    }
}

TEST(InstanceTest, NoExceptionWhenCorrectUsage) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .addPortability()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance->findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    // Create a valid buffer - should not throw
    vk::BufferCreateInfo validBufferInfo;
    validBufferInfo.setSize(1024)  // Valid size
        .setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
        .setSharingMode(vk::SharingMode::eExclusive);

    vk::Buffer buffer;
    auto createBuffer = [&]() {
        auto [result, buf] = device->handle().createBuffer(validBufferInfo);
        EXPECT_EQ(result, vk::Result::eSuccess);
        buffer = buf;
    };
    EXPECT_NO_THROW(createBuffer());

    // Clean up
    if (buffer) {
        device->handle().destroyBuffer(buffer);
    }
}
