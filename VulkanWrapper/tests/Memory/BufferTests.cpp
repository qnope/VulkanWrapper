#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/BufferList.h"
#include <cstring>
#include <gtest/gtest.h>
#include <vector>

TEST(BufferTest, CreateUniformBuffer) {
    auto &gpu = vw::tests::create_gpu();
    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100);

    EXPECT_EQ(buffer.size(), 100);
    EXPECT_GT(buffer.size_bytes(), 0);
    SUCCEED();
}

TEST(BufferTest, CreateStorageBuffer) {
    auto &gpu = vw::tests::create_gpu();
    constexpr VkBufferUsageFlags StorageBufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using StorageBuffer = vw::Buffer<uint32_t, false, StorageBufferUsage>;
    auto buffer = vw::create_buffer<StorageBuffer>(*gpu.allocator, 50);

    EXPECT_EQ(buffer.size(), 50);
    EXPECT_EQ(buffer.size_bytes(), 50 * sizeof(uint32_t));
    SUCCEED();
}

TEST(BufferTest, CreateHostVisibleBuffer) {
    auto &gpu = vw::tests::create_gpu();
    using HostUniformBuffer = vw::Buffer<float, true, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<HostUniformBuffer>(*gpu.allocator, 100);

    EXPECT_EQ(buffer.size(), 100);
    SUCCEED();
}

TEST(BufferTest, CopySingleElementToHostVisibleBuffer) {
    auto &gpu = vw::tests::create_gpu();
    using HostUniformBuffer = vw::Buffer<float, true, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<HostUniformBuffer>(*gpu.allocator, 10);

    float value = 42.5f;
    buffer.copy(value, 0);

    auto retrieved = buffer.as_vector(0, 1);
    ASSERT_EQ(retrieved.size(), 1);
    EXPECT_FLOAT_EQ(retrieved[0], value);
}

TEST(BufferTest, CopyMultipleElementsToHostVisibleBuffer) {
    auto &gpu = vw::tests::create_gpu();
    using HostUniformBuffer = vw::Buffer<float, true, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<HostUniformBuffer>(*gpu.allocator, 100);

    std::vector<float> values = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    buffer.copy(std::span<const float>(values), 0);

    auto retrieved = buffer.as_vector(0, values.size());
    ASSERT_EQ(retrieved.size(), values.size());

    for (size_t i = 0; i < values.size(); ++i) {
        EXPECT_FLOAT_EQ(retrieved[i], values[i]);
    }
}

TEST(BufferTest, CopyWithOffset) {
    auto &gpu = vw::tests::create_gpu();
    using HostUniformBuffer = vw::Buffer<int32_t, true, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<HostUniformBuffer>(*gpu.allocator, 20);

    std::vector<int32_t> values1 = {10, 20, 30};
    std::vector<int32_t> values2 = {40, 50, 60};

    buffer.copy(std::span<const int32_t>(values1), 0);
    buffer.copy(std::span<const int32_t>(values2), 5);

    auto retrieved1 = buffer.as_vector(0, 3);
    auto retrieved2 = buffer.as_vector(5, 3);

    ASSERT_EQ(retrieved1.size(), 3);
    ASSERT_EQ(retrieved2.size(), 3);

    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(retrieved1[i], values1[i]);
        EXPECT_EQ(retrieved2[i], values2[i]);
    }
}

TEST(BufferTest, CreateBufferWithStruct) {
    struct DataStruct {
        float x, y, z;
        float r, g, b;
    };

    auto &gpu = vw::tests::create_gpu();
    using HostStructBuffer =
        vw::Buffer<DataStruct, true, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<HostStructBuffer>(*gpu.allocator, 50);

    EXPECT_EQ(buffer.size(), 50);

    DataStruct v{1.0f, 2.0f, 3.0f, 0.5f, 0.5f, 0.5f};
    buffer.copy(v, 0);

    auto retrieved = buffer.as_vector(0, 1);
    ASSERT_EQ(retrieved.size(), 1);

    EXPECT_FLOAT_EQ(retrieved[0].x, v.x);
    EXPECT_FLOAT_EQ(retrieved[0].y, v.y);
    EXPECT_FLOAT_EQ(retrieved[0].z, v.z);
    EXPECT_FLOAT_EQ(retrieved[0].r, v.r);
    EXPECT_FLOAT_EQ(retrieved[0].g, v.g);
    EXPECT_FLOAT_EQ(retrieved[0].b, v.b);
}

TEST(BufferTest, MoveBuffer) {
    auto &gpu = vw::tests::create_gpu();
    using HostUniformBuffer = vw::Buffer<float, true, vw::UniformBufferUsage>;
    auto buffer1 = vw::create_buffer<HostUniformBuffer>(*gpu.allocator, 10);

    float value = 123.456f;
    buffer1.copy(value, 0);

    auto buffer2 = std::move(buffer1);

    auto retrieved = buffer2.as_vector(0, 1);
    ASSERT_EQ(retrieved.size(), 1);
    EXPECT_FLOAT_EQ(retrieved[0], value);
}

TEST(BufferTest, CreateLargeBuffer) {
    auto &gpu = vw::tests::create_gpu();
    constexpr size_t largeSize = 1024 * 1024;
    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, largeSize);

    EXPECT_EQ(buffer.size(), largeSize);
    EXPECT_EQ(buffer.size_bytes(), largeSize * sizeof(float));
}

TEST(BufferTest, GenericCopy) {
    auto &gpu = vw::tests::create_gpu();
    using HostByteBuffer = vw::Buffer<std::byte, true, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<HostByteBuffer>(*gpu.allocator, 100);

    std::vector<uint32_t> data = {0x12345678, 0xABCDEF00, 0xDEADBEEF};
    buffer.generic_copy(std::span<const uint32_t>(data), 0);

    auto retrieved = buffer.as_vector(0, data.size() * sizeof(uint32_t));

    std::vector<uint32_t> retrieved_data(data.size());
    std::memcpy(retrieved_data.data(), retrieved.data(),
                data.size() * sizeof(uint32_t));

    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(retrieved_data[i], data[i]);
    }
}

// BufferList tests

TEST(BufferListTest, FirstAllocationStartsAtZero) {
    auto &gpu = vw::tests::create_gpu();
    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;

    StorageBufferList list(gpu.allocator);

    auto info = list.create_buffer(100);

    EXPECT_EQ(info.offset, 0);
    EXPECT_NE(info.buffer, nullptr);
}

TEST(BufferListTest, FirstAllocationWithAlignmentStartsAtZero) {
    auto &gpu = vw::tests::create_gpu();
    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;

    StorageBufferList list(gpu.allocator);

    // Even with 256-byte alignment, first allocation should start at 0
    auto info = list.create_buffer(100, 256);

    EXPECT_EQ(info.offset, 0);
    EXPECT_NE(info.buffer, nullptr);
}

TEST(BufferListTest, SecondAllocationWithoutAlignment) {
    auto &gpu = vw::tests::create_gpu();
    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;

    StorageBufferList list(gpu.allocator);

    auto info1 = list.create_buffer(100);
    auto info2 = list.create_buffer(50);

    // Without alignment, second allocation should start immediately after first
    EXPECT_EQ(info1.offset, 0);
    EXPECT_EQ(info2.offset, 100);
    // Both should use the same underlying buffer
    EXPECT_EQ(info1.buffer, info2.buffer);
}

TEST(BufferListTest, SecondAllocationWithAlignment) {
    auto &gpu = vw::tests::create_gpu();
    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;

    StorageBufferList list(gpu.allocator);

    // First allocation: 100 bytes starting at 0
    auto info1 = list.create_buffer(100);
    // Second allocation with 256-byte alignment
    auto info2 = list.create_buffer(50, 256);

    EXPECT_EQ(info1.offset, 0);
    // Second allocation should be aligned to 256 bytes
    EXPECT_EQ(info2.offset, 256);
    EXPECT_EQ(info2.offset % 256, 0);
    // Both should use the same underlying buffer
    EXPECT_EQ(info1.buffer, info2.buffer);
}

TEST(BufferListTest, MultipleAllocationsWithAlignment) {
    auto &gpu = vw::tests::create_gpu();
    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;

    StorageBufferList list(gpu.allocator);

    // Allocate several buffers with 256-byte alignment
    auto info1 = list.create_buffer(100, 256); // 0-99, next at 100
    auto info2 = list.create_buffer(200, 256); // aligned to 256, 256-455
    auto info3 = list.create_buffer(50, 256);  // aligned to 512, 512-561

    EXPECT_EQ(info1.offset, 0);
    EXPECT_EQ(info2.offset, 256);
    EXPECT_EQ(info3.offset, 512);

    // All offsets should be 256-byte aligned
    EXPECT_EQ(info1.offset % 256, 0);
    EXPECT_EQ(info2.offset % 256, 0);
    EXPECT_EQ(info3.offset % 256, 0);
}

TEST(BufferListTest, MixedAlignmentAllocations) {
    auto &gpu = vw::tests::create_gpu();
    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;

    StorageBufferList list(gpu.allocator);

    // Mix of aligned and unaligned allocations
    auto info1 = list.create_buffer(100);     // 0-99, next at 100
    auto info2 = list.create_buffer(50);      // 100-149, next at 150
    auto info3 = list.create_buffer(30, 256); // aligned to 256, 256-285

    EXPECT_EQ(info1.offset, 0);
    EXPECT_EQ(info2.offset, 100);
    EXPECT_EQ(info3.offset, 256);
    EXPECT_EQ(info3.offset % 256, 0);
}

TEST(BufferListTest, AlignmentWithVariousPowersOfTwo) {
    auto &gpu = vw::tests::create_gpu();
    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;

    StorageBufferList list(gpu.allocator);

    auto info1 = list.create_buffer(10);      // 0-9, next at 10
    auto info2 = list.create_buffer(10, 16);  // aligned to 16, 16-25
    auto info3 = list.create_buffer(10, 64);  // aligned to 64, 64-73
    auto info4 = list.create_buffer(10, 128); // aligned to 128, 128-137

    EXPECT_EQ(info1.offset, 0);
    EXPECT_EQ(info2.offset, 16);
    EXPECT_EQ(info3.offset, 64);
    EXPECT_EQ(info4.offset, 128);

    EXPECT_EQ(info2.offset % 16, 0);
    EXPECT_EQ(info3.offset % 64, 0);
    EXPECT_EQ(info4.offset % 128, 0);
}

TEST(BufferListTest, AlignmentDoesNotOverlapPreviousAllocation) {
    auto &gpu = vw::tests::create_gpu();
    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;

    StorageBufferList list(gpu.allocator);

    // Allocate 200 bytes, which goes past 128 but not to 256
    auto info1 = list.create_buffer(200); // 0-199, next at 200
    // Next allocation with 128 alignment should go to 256, not 128
    auto info2 = list.create_buffer(
        50, 128); // aligned to 256 (next multiple of 128 >= 200)

    EXPECT_EQ(info1.offset, 0);
    EXPECT_EQ(info2.offset, 256); // 256 is the next multiple of 128 after 200
    EXPECT_EQ(info2.offset % 128, 0);
    EXPECT_GE(info2.offset, 200); // Must not overlap with first allocation
}
