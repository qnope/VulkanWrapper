#include "VulkanWrapper/Vulkan/Instance.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include <gtest/gtest.h>

TEST(InstanceTest, CreateInstance) {
    auto instance = vw::InstanceBuilder()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    EXPECT_NE(instance->handle(), nullptr);
}

TEST(InstanceTest, FindGpu) {
    auto instance = vw::InstanceBuilder()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto deviceFinder = instance->findGpu();

    // Just verify that findGpu returns a DeviceFinder
    // We can't test much more without actually building a device
    SUCCEED();
}
