#include "VulkanWrapper/Vulkan/Instance.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include <gtest/gtest.h>

TEST(InstanceTest, CreateInstance) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    EXPECT_NE(instance.handle(), nullptr);
}

TEST(InstanceTest, CopyInstance) {
    auto instance1 = vw::InstanceBuilder()
                         .setDebug()
                         .setApiVersion(vw::ApiVersion::e13)
                         .build();

    auto handle = instance1.handle();

    // Test copy constructor
    vw::Instance instance2 = instance1;

    EXPECT_EQ(instance2.handle(), handle);
    EXPECT_EQ(instance1.handle(), handle);
}

TEST(InstanceTest, CopyAssignmentInstance) {
    auto instance1 = vw::InstanceBuilder()
                         .setDebug()
                         .setApiVersion(vw::ApiVersion::e13)
                         .build();

    auto instance2 = vw::InstanceBuilder()
                         .setDebug()
                         .setApiVersion(vw::ApiVersion::e10)
                         .build();

    auto handle1 = instance1.handle();

    // Test copy assignment
    instance2 = instance1;

    EXPECT_EQ(instance2.handle(), handle1);
    EXPECT_EQ(instance1.handle(), handle1);
}

TEST(InstanceTest, MoveInstance) {
    auto instance1 = vw::InstanceBuilder()
                         .setDebug()
                         .setApiVersion(vw::ApiVersion::e13)
                         .build();

    auto handle = instance1.handle();

    // Test move constructor
    vw::Instance instance2 = std::move(instance1);

    EXPECT_EQ(instance2.handle(), handle);
}

TEST(InstanceTest, FindGpu) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto deviceFinder = instance.findGpu();

    // Just verify that findGpu returns a DeviceFinder
    // We can't test much more without actually building a device
    SUCCEED();
}
