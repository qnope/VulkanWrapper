#include <gtest/gtest.h>
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include "utils/create_gpu.hpp"

TEST(AllocatorTest, CreateAllocator) {
    auto gpu = vw::tests::create_gpu();
    EXPECT_NE(gpu.allocator.handle(), nullptr);
    SUCCEED();
}

TEST(AllocatorTest, AllocateUniformBuffer) {
    auto gpu = vw::tests::create_gpu();
    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer = gpu.allocator.create_buffer<UniformBuffer>(100);

    EXPECT_EQ(buffer.size(), 100);
    EXPECT_TRUE(buffer.does_support(vk::BufferUsageFlagBits::eUniformBuffer));
    EXPECT_TRUE(buffer.does_support(vk::BufferUsageFlagBits::eTransferDst));
}

TEST(AllocatorTest, AllocateHostVisibleUniformBuffer) {
    auto gpu = vw::tests::create_gpu();
    using HostUniformBuffer = vw::Buffer<float, true, vw::UniformBufferUsage>;
    auto buffer = gpu.allocator.create_buffer<HostUniformBuffer>(100);

    EXPECT_EQ(buffer.size(), 100);
    EXPECT_TRUE(buffer.host_visible);
}

TEST(AllocatorTest, AllocateStorageBuffer) {
    auto gpu = vw::tests::create_gpu();
    constexpr VkBufferUsageFlags StorageBufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using StorageBuffer = vw::Buffer<uint32_t, false, StorageBufferUsage>;
    auto buffer = gpu.allocator.create_buffer<StorageBuffer>(50);

    EXPECT_EQ(buffer.size(), 50);
    EXPECT_TRUE(buffer.does_support(vk::BufferUsageFlagBits::eStorageBuffer));
}

TEST(AllocatorTest, AllocateMultipleBuffers) {
    auto gpu = vw::tests::create_gpu();

    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    constexpr VkBufferUsageFlags StorageBufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using StorageBuffer = vw::Buffer<uint32_t, false, StorageBufferUsage>;

    auto buffer1 = gpu.allocator.create_buffer<UniformBuffer>(100);
    auto buffer2 = gpu.allocator.create_buffer<UniformBuffer>(200);
    auto buffer3 = gpu.allocator.create_buffer<StorageBuffer>(150);

    EXPECT_EQ(buffer1.size(), 100);
    EXPECT_EQ(buffer2.size(), 200);
    EXPECT_EQ(buffer3.size(), 150);
}

TEST(AllocatorTest, CreateBufferWithCustomUsage) {
    auto gpu = vw::tests::create_gpu();

    using CustomBuffer = vw::Buffer<uint32_t, true, vw::UniformBufferUsage>;
    auto buffer = gpu.allocator.create_buffer<CustomBuffer>(20);

    EXPECT_EQ(buffer.size(), 20);
    EXPECT_TRUE(buffer.does_support(vk::BufferUsageFlagBits::eUniformBuffer));
    EXPECT_TRUE(buffer.host_visible);
}

TEST(AllocatorTest, CreateImage2D) {
    auto gpu = vw::tests::create_gpu();

    auto image = gpu.allocator.create_image_2D(
        vw::Width{256},
        vw::Height{256},
        false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->format(), vk::Format::eR8G8B8A8Unorm);
    EXPECT_EQ(image->extent2D().width, 256);
    EXPECT_EQ(image->extent2D().height, 256);
}

TEST(AllocatorTest, CreateImage2DWithMipmaps) {
    auto gpu = vw::tests::create_gpu();

    auto image = gpu.allocator.create_image_2D(
        vw::Width{512},
        vw::Height{512},
        true,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    EXPECT_GT(static_cast<uint32_t>(image->mip_levels()), 1);
}

TEST(AllocatorTest, CreateDifferentImageFormats) {
    auto gpu = vw::tests::create_gpu();

    auto imageRGBA = gpu.allocator.create_image_2D(
        vw::Width{128},
        vw::Height{128},
        false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto imageDepth = gpu.allocator.create_image_2D(
        vw::Width{128},
        vw::Height{128},
        false,
        vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment
    );

    ASSERT_NE(imageRGBA, nullptr);
    ASSERT_NE(imageDepth, nullptr);

    EXPECT_EQ(imageRGBA->format(), vk::Format::eR8G8B8A8Unorm);
    EXPECT_EQ(imageDepth->format(), vk::Format::eD32Sfloat);
}

TEST(AllocatorTest, MoveAllocator) {
    auto gpu = vw::tests::create_gpu();
    auto handle = gpu.allocator.handle();

    vw::Allocator allocator2 = std::move(gpu.allocator);

    EXPECT_EQ(allocator2.handle(), handle);
}

TEST(AllocatorTest, AllocatorBuilder) {
    auto instance = vw::InstanceBuilder()
                        .setDebug()
                        .setApiVersion(vw::ApiVersion::e13)
                        .build();

    auto device = instance.findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    auto allocator = vw::AllocatorBuilder(instance, device).build();

    EXPECT_NE(allocator.handle(), nullptr);

    auto buffer = allocator.allocate_vertex_buffer<float>(10);
    EXPECT_EQ(buffer.size(), 10);
}

TEST(AllocatorTest, CreateTypedBuffer) {
    auto gpu = vw::tests::create_gpu();

    using VertexBuffer = vw::Buffer<float, false, vw::VertexBufferUsage>;
    auto buffer = gpu.allocator.create_buffer<VertexBuffer>(100);

    EXPECT_EQ(buffer.size(), 100);
    EXPECT_FALSE(buffer.host_visible);
    EXPECT_TRUE(buffer.does_support(vk::BufferUsageFlagBits::eVertexBuffer));
}

TEST(AllocatorTest, CreateMultipleImages) {
    auto gpu = vw::tests::create_gpu();

    auto image1 = gpu.allocator.create_image_2D(
        vw::Width{256}, vw::Height{256}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto image2 = gpu.allocator.create_image_2D(
        vw::Width{512}, vw::Height{512}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image1, nullptr);
    ASSERT_NE(image2, nullptr);

    EXPECT_NE(image1->handle(), image2->handle());
}
