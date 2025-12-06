#include <gtest/gtest.h>
#include "VulkanWrapper/Memory/Transfer.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageLoader.h"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include "utils/create_gpu.hpp"
#include <vector>
#include <cstring>
#include <filesystem>

TEST(TransferTest, CreateTransfer) {
    vw::Transfer transfer;
    SUCCEED();
}

TEST(TransferTest, CopyBuffer) {
    auto& gpu = vw::tests::create_gpu();

    // Create source and destination buffers
    constexpr VkBufferUsageFlags BufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using TestBuffer = vw::Buffer<float, true, BufferUsage>;

    auto srcBuffer = vw::create_buffer<TestBuffer>(*gpu.allocator, 10);
    auto dstBuffer = vw::create_buffer<TestBuffer>(*gpu.allocator, 10);

    // Fill source buffer
    std::vector<float> testData = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    srcBuffer.copy(std::span<const float>(testData), 0);

    // Perform transfer
    auto cmdPool = vw::CommandPoolBuilder(gpu.device).build();
    auto cmd = cmdPool.allocate(1)[0];

    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer;
    transfer.copyBuffer(cmd, srcBuffer.handle(), dstBuffer.handle(),
                        0, 0, testData.size() * sizeof(float));

    std::ignore = cmd.end();

    gpu.queue().enqueue_command_buffer(cmd);
    gpu.queue().submit({}, {}, {}).wait();

    // Verify
    auto retrieved = dstBuffer.as_vector(0, testData.size());
    ASSERT_EQ(retrieved.size(), testData.size());

    for (size_t i = 0; i < testData.size(); ++i) {
        EXPECT_FLOAT_EQ(retrieved[i], testData[i])
            << "Mismatch at index " << i;
    }
}

TEST(TransferTest, CopyBufferWithOffset) {
    auto& gpu = vw::tests::create_gpu();

    constexpr VkBufferUsageFlags BufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using TestBuffer = vw::Buffer<float, true, BufferUsage>;

    auto srcBuffer = vw::create_buffer<TestBuffer>(*gpu.allocator, 20);
    auto dstBuffer = vw::create_buffer<TestBuffer>(*gpu.allocator, 20);

    // Fill source buffer starting at offset 5
    std::vector<float> testData = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    srcBuffer.copy(std::span<const float>(testData), 5);

    auto cmdPool = vw::CommandPoolBuilder(gpu.device).build();
    auto cmd = cmdPool.allocate(1)[0];

    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer;
    // Copy from offset 5 in source to offset 10 in destination
    transfer.copyBuffer(cmd, srcBuffer.handle(), dstBuffer.handle(),
                        5 * sizeof(float), 10 * sizeof(float),
                        testData.size() * sizeof(float));

    std::ignore = cmd.end();

    gpu.queue().enqueue_command_buffer(cmd);
    gpu.queue().submit({}, {}, {}).wait();

    // Verify data at offset 10
    auto retrieved = dstBuffer.as_vector(10, testData.size());
    ASSERT_EQ(retrieved.size(), testData.size());

    for (size_t i = 0; i < testData.size(); ++i) {
        EXPECT_FLOAT_EQ(retrieved[i], testData[i])
            << "Mismatch at index " << i;
    }
}

TEST(TransferTest, CopyBufferToImage) {
    auto& gpu = vw::tests::create_gpu();

    constexpr uint32_t width = 64;
    constexpr uint32_t height = 64;
    constexpr size_t pixelCount = width * height;
    constexpr size_t bufferSize = pixelCount * 4; // RGBA8

    // Create staging buffer with pixel data
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto stagingBuffer = vw::create_buffer<StagingBuffer>(*gpu.allocator, bufferSize);

    // Fill with test pattern (red gradient)
    std::vector<std::byte> pixels(bufferSize);
    for (size_t i = 0; i < pixelCount; ++i) {
        pixels[i * 4 + 0] = static_cast<std::byte>(i % 256);  // R
        pixels[i * 4 + 1] = static_cast<std::byte>(0);        // G
        pixels[i * 4 + 2] = static_cast<std::byte>(0);        // B
        pixels[i * 4 + 3] = static_cast<std::byte>(255);      // A
    }
    stagingBuffer.copy(std::span<const std::byte>(pixels), 0);

    // Create destination image
    auto image = gpu.allocator->create_image_2D(
        vw::Width{width}, vw::Height{height}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
    );

    auto cmdPool = vw::CommandPoolBuilder(gpu.device).build();
    auto cmd = cmdPool.allocate(1)[0];

    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer;
    transfer.copyBufferToImage(cmd, stagingBuffer.handle(), image, 0);

    std::ignore = cmd.end();

    gpu.queue().enqueue_command_buffer(cmd);
    gpu.queue().submit({}, {}, {}).wait();

    SUCCEED();
}

TEST(TransferTest, CopyImageToBuffer) {
    auto& gpu = vw::tests::create_gpu();

    constexpr uint32_t width = 64;
    constexpr uint32_t height = 64;
    constexpr size_t pixelCount = width * height;
    constexpr size_t bufferSize = pixelCount * 4; // RGBA8

    // Create source image
    auto image = gpu.allocator->create_image_2D(
        vw::Width{width}, vw::Height{height}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
    );

    // Create staging buffer with pixel data to fill the image
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto stagingBufferSrc = vw::create_buffer<StagingBuffer>(*gpu.allocator, bufferSize);

    // Fill with test pattern
    std::vector<std::byte> sourcePixels(bufferSize);
    for (size_t i = 0; i < pixelCount; ++i) {
        sourcePixels[i * 4 + 0] = static_cast<std::byte>(i % 256);  // R
        sourcePixels[i * 4 + 1] = static_cast<std::byte>((i / 64) % 256);  // G
        sourcePixels[i * 4 + 2] = static_cast<std::byte>(128);      // B
        sourcePixels[i * 4 + 3] = static_cast<std::byte>(255);      // A
    }
    stagingBufferSrc.copy(std::span<const std::byte>(sourcePixels), 0);

    // First, copy buffer to image
    auto cmdPool = vw::CommandPoolBuilder(gpu.device).build();
    auto cmd1 = cmdPool.allocate(1)[0];

    std::ignore = cmd1.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer1;
    transfer1.copyBufferToImage(cmd1, stagingBufferSrc.handle(), image, 0);

    std::ignore = cmd1.end();

    gpu.queue().enqueue_command_buffer(cmd1);
    gpu.queue().submit({}, {}, {}).wait();

    // Create destination buffer
    auto stagingBufferDst = vw::create_buffer<StagingBuffer>(*gpu.allocator, bufferSize);

    // Now copy image back to buffer
    auto cmd2 = cmdPool.allocate(1)[0];

    std::ignore = cmd2.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer2;
    transfer2.copyImageToBuffer(cmd2, image, stagingBufferDst.handle(), 0);

    std::ignore = cmd2.end();

    gpu.queue().enqueue_command_buffer(cmd2);
    gpu.queue().submit({}, {}, {}).wait();

    // Verify the data matches
    auto retrieved = stagingBufferDst.as_vector(0, bufferSize);
    ASSERT_EQ(retrieved.size(), sourcePixels.size());

    for (size_t i = 0; i < sourcePixels.size(); ++i) {
        EXPECT_EQ(retrieved[i], sourcePixels[i])
            << "Mismatch at byte " << i;
    }
}

TEST(TransferTest, BlitImage) {
    auto& gpu = vw::tests::create_gpu();

    constexpr uint32_t srcWidth = 128;
    constexpr uint32_t srcHeight = 128;
    constexpr uint32_t dstWidth = 64;
    constexpr uint32_t dstHeight = 64;

    // Create source and destination images
    auto srcImage = gpu.allocator->create_image_2D(
        vw::Width{srcWidth}, vw::Height{srcHeight}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
    );

    auto dstImage = gpu.allocator->create_image_2D(
        vw::Width{dstWidth}, vw::Height{dstHeight}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
    );

    // Fill source image with test data
    constexpr size_t srcBufferSize = srcWidth * srcHeight * 4;
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto stagingBuffer = vw::create_buffer<StagingBuffer>(*gpu.allocator, srcBufferSize);

    std::vector<std::byte> srcPixels(srcBufferSize);
    for (size_t i = 0; i < srcWidth * srcHeight; ++i) {
        srcPixels[i * 4 + 0] = static_cast<std::byte>(255);  // R
        srcPixels[i * 4 + 1] = static_cast<std::byte>(0);    // G
        srcPixels[i * 4 + 2] = static_cast<std::byte>(0);    // B
        srcPixels[i * 4 + 3] = static_cast<std::byte>(255);  // A
    }
    stagingBuffer.copy(std::span<const std::byte>(srcPixels), 0);

    auto cmdPool = vw::CommandPoolBuilder(gpu.device).build();
    auto cmd1 = cmdPool.allocate(1)[0];

    std::ignore = cmd1.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer1;
    transfer1.copyBufferToImage(cmd1, stagingBuffer.handle(), srcImage, 0);

    std::ignore = cmd1.end();

    gpu.queue().enqueue_command_buffer(cmd1);
    gpu.queue().submit({}, {}, {}).wait();

    // Perform blit
    auto cmd2 = cmdPool.allocate(1)[0];

    std::ignore = cmd2.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer2;
    transfer2.blit(cmd2, srcImage, dstImage);

    std::ignore = cmd2.end();

    gpu.queue().enqueue_command_buffer(cmd2);
    gpu.queue().submit({}, {}, {}).wait();

    SUCCEED();
}

TEST(TransferTest, CopyImageToBufferAndSaveToDisk) {
    auto& gpu = vw::tests::create_gpu();

    constexpr uint32_t width = 64;
    constexpr uint32_t height = 64;
    constexpr size_t pixelCount = width * height;
    constexpr size_t bufferSize = pixelCount * 4; // RGBA8

    // Create source image with test data
    auto image = gpu.allocator->create_image_2D(
        vw::Width{width}, vw::Height{height}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
    );

    // Create staging buffer with pixel data
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto stagingBufferSrc = vw::create_buffer<StagingBuffer>(*gpu.allocator, bufferSize);

    // Create a gradient pattern
    std::vector<std::byte> sourcePixels(bufferSize);
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            size_t i = y * width + x;
            sourcePixels[i * 4 + 0] = static_cast<std::byte>((x * 255) / width);   // R gradient
            sourcePixels[i * 4 + 1] = static_cast<std::byte>((y * 255) / height);  // G gradient
            sourcePixels[i * 4 + 2] = static_cast<std::byte>(128);                 // B constant
            sourcePixels[i * 4 + 3] = static_cast<std::byte>(255);                 // A opaque
        }
    }
    stagingBufferSrc.copy(std::span<const std::byte>(sourcePixels), 0);

    // Copy buffer to image
    auto cmdPool = vw::CommandPoolBuilder(gpu.device).build();
    auto cmd1 = cmdPool.allocate(1)[0];

    std::ignore = cmd1.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer1;
    transfer1.copyBufferToImage(cmd1, stagingBufferSrc.handle(), image, 0);

    std::ignore = cmd1.end();

    gpu.queue().enqueue_command_buffer(cmd1);
    gpu.queue().submit({}, {}, {}).wait();

    // Copy image to buffer
    auto stagingBufferDst = vw::create_buffer<StagingBuffer>(*gpu.allocator, bufferSize);

    auto cmd2 = cmdPool.allocate(1)[0];

    std::ignore = cmd2.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer2;
    transfer2.copyImageToBuffer(cmd2, image, stagingBufferDst.handle(), 0);

    std::ignore = cmd2.end();

    gpu.queue().enqueue_command_buffer(cmd2);
    gpu.queue().submit({}, {}, {}).wait();

    // Get pixel data and save to file
    auto retrievedPixels = stagingBufferDst.as_vector(0, bufferSize);

    // Save to a temporary file
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "transfer_test_output.png";

    vw::save_image(tempPath, vw::Width{width}, vw::Height{height},
                   std::span<const std::byte>(retrievedPixels));

    // Verify file was created
    ASSERT_TRUE(std::filesystem::exists(tempPath));

    // Clean up
    std::filesystem::remove(tempPath);
}

TEST(TransferTest, ResourceTrackerAccess) {
    vw::Transfer transfer;

    // Verify we can access the resource tracker
    auto& tracker = transfer.resourceTracker();
    (void)tracker; // Just verify access works

    SUCCEED();
}

TEST(TransferTest, MultipleTransferOperations) {
    auto& gpu = vw::tests::create_gpu();

    constexpr VkBufferUsageFlags BufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using TestBuffer = vw::Buffer<float, true, BufferUsage>;

    auto buffer1 = vw::create_buffer<TestBuffer>(*gpu.allocator, 10);
    auto buffer2 = vw::create_buffer<TestBuffer>(*gpu.allocator, 10);
    auto buffer3 = vw::create_buffer<TestBuffer>(*gpu.allocator, 10);

    // Fill buffer1
    std::vector<float> data1 = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    buffer1.copy(std::span<const float>(data1), 0);

    auto cmdPool = vw::CommandPoolBuilder(gpu.device).build();
    auto cmd = cmdPool.allocate(1)[0];

    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Multiple transfer operations with same Transfer object
    vw::Transfer transfer;
    transfer.copyBuffer(cmd, buffer1.handle(), buffer2.handle(),
                        0, 0, data1.size() * sizeof(float));
    transfer.copyBuffer(cmd, buffer1.handle(), buffer3.handle(),
                        0, 0, data1.size() * sizeof(float));

    std::ignore = cmd.end();

    gpu.queue().enqueue_command_buffer(cmd);
    gpu.queue().submit({}, {}, {}).wait();

    // Verify both buffers
    auto retrieved2 = buffer2.as_vector(0, data1.size());
    auto retrieved3 = buffer3.as_vector(0, data1.size());

    for (size_t i = 0; i < data1.size(); ++i) {
        EXPECT_FLOAT_EQ(retrieved2[i], data1[i]) << "Buffer2 mismatch at index " << i;
        EXPECT_FLOAT_EQ(retrieved3[i], data1[i]) << "Buffer3 mismatch at index " << i;
    }
}
