// Template: Buffer Test
// Usage: Copy this file for testing buffer operations

#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/BufferList.h"
#include <cstring>
#include <gtest/gtest.h>
#include <vector>

namespace {

// ============================================================================
// Test Constants
// ============================================================================

constexpr size_t SMALL_SIZE = 10;
constexpr size_t MEDIUM_SIZE = 100;
constexpr size_t LARGE_SIZE = 1024 * 1024;

// ============================================================================
// Buffer Type Aliases
// ============================================================================

// Device-local buffers
using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
using StorageBuffer =
    vw::Buffer<uint32_t, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;

// Host-visible buffers (CPU accessible)
using HostUniformBuffer = vw::Buffer<float, true, vw::UniformBufferUsage>;
using HostStorageBuffer =
    vw::Buffer<uint32_t, true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;

// Staging buffers
using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;

// ============================================================================
// Creation Tests
// ============================================================================

TEST(YourBufferTest, CreateWithValidSize) {
    auto &gpu = vw::tests::create_gpu();

    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, MEDIUM_SIZE);

    EXPECT_EQ(buffer.size(), MEDIUM_SIZE);
    EXPECT_GT(buffer.size_bytes(), 0);
    EXPECT_TRUE(buffer.handle());
}

TEST(YourBufferTest, CreateHostVisibleBuffer) {
    auto &gpu = vw::tests::create_gpu();

    auto buffer =
        vw::create_buffer<HostUniformBuffer>(*gpu.allocator, MEDIUM_SIZE);

    EXPECT_EQ(buffer.size(), MEDIUM_SIZE);
    EXPECT_TRUE(buffer.handle());
}

TEST(YourBufferTest, CreateLargeBuffer) {
    auto &gpu = vw::tests::create_gpu();

    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, LARGE_SIZE);

    EXPECT_EQ(buffer.size(), LARGE_SIZE);
    EXPECT_EQ(buffer.size_bytes(), LARGE_SIZE * sizeof(float));
}

// ============================================================================
// Write/Read Tests
// ============================================================================

TEST(YourBufferTest, WriteSingleElement) {
    auto &gpu = vw::tests::create_gpu();

    auto buffer =
        vw::create_buffer<HostUniformBuffer>(*gpu.allocator, SMALL_SIZE);

    float value = 42.5f;
    buffer.write(value, 0);

    auto retrieved = buffer.read_as_vector(0, 1);
    ASSERT_EQ(retrieved.size(), 1);
    EXPECT_FLOAT_EQ(retrieved[0], value);
}

TEST(YourBufferTest, WriteMultipleElements) {
    auto &gpu = vw::tests::create_gpu();

    auto buffer =
        vw::create_buffer<HostUniformBuffer>(*gpu.allocator, MEDIUM_SIZE);

    std::vector<float> values = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    buffer.write(std::span<const float>(values), 0);

    auto retrieved = buffer.read_as_vector(0, values.size());
    ASSERT_EQ(retrieved.size(), values.size());

    for (size_t i = 0; i < values.size(); ++i) {
        EXPECT_FLOAT_EQ(retrieved[i], values[i]);
    }
}

TEST(YourBufferTest, WriteWithOffset) {
    auto &gpu = vw::tests::create_gpu();

    using HostIntBuffer =
        vw::Buffer<int32_t, true, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<HostIntBuffer>(*gpu.allocator, 20);

    std::vector<int32_t> values1 = {10, 20, 30};
    std::vector<int32_t> values2 = {40, 50, 60};

    buffer.write(std::span<const int32_t>(values1), 0);
    buffer.write(std::span<const int32_t>(values2), 5);

    auto retrieved1 = buffer.read_as_vector(0, 3);
    auto retrieved2 = buffer.read_as_vector(5, 3);

    ASSERT_EQ(retrieved1.size(), 3);
    ASSERT_EQ(retrieved2.size(), 3);

    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(retrieved1[i], values1[i]);
        EXPECT_EQ(retrieved2[i], values2[i]);
    }
}

// ============================================================================
// Struct Buffer Tests
// ============================================================================

TEST(YourBufferTest, CreateBufferWithStruct) {
    struct MyData {
        float x, y, z;
        float r, g, b;
    };

    auto &gpu = vw::tests::create_gpu();

    using HostStructBuffer = vw::Buffer<MyData, true, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<HostStructBuffer>(*gpu.allocator, 50);

    EXPECT_EQ(buffer.size(), 50);

    MyData data{1.0f, 2.0f, 3.0f, 0.5f, 0.5f, 0.5f};
    buffer.write(data, 0);

    auto retrieved = buffer.read_as_vector(0, 1);
    ASSERT_EQ(retrieved.size(), 1);

    EXPECT_FLOAT_EQ(retrieved[0].x, data.x);
    EXPECT_FLOAT_EQ(retrieved[0].y, data.y);
    EXPECT_FLOAT_EQ(retrieved[0].z, data.z);
    EXPECT_FLOAT_EQ(retrieved[0].r, data.r);
    EXPECT_FLOAT_EQ(retrieved[0].g, data.g);
    EXPECT_FLOAT_EQ(retrieved[0].b, data.b);
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST(YourBufferTest, MoveBuffer) {
    auto &gpu = vw::tests::create_gpu();

    auto buffer1 =
        vw::create_buffer<HostUniformBuffer>(*gpu.allocator, SMALL_SIZE);

    float value = 123.456f;
    buffer1.write(value, 0);

    auto buffer2 = std::move(buffer1);

    auto retrieved = buffer2.read_as_vector(0, 1);
    ASSERT_EQ(retrieved.size(), 1);
    EXPECT_FLOAT_EQ(retrieved[0], value);
}

// ============================================================================
// Raw Bytes Tests
// ============================================================================

TEST(YourBufferTest, WriteBytes) {
    auto &gpu = vw::tests::create_gpu();

    auto buffer = vw::create_buffer<StagingBuffer>(*gpu.allocator, 100);

    std::vector<uint32_t> data = {0x12345678, 0xABCDEF00, 0xDEADBEEF};
    buffer.write_bytes(std::span<const uint32_t>(data), 0);

    auto retrieved = buffer.read_as_vector(0, data.size() * sizeof(uint32_t));

    std::vector<uint32_t> retrieved_data(data.size());
    std::memcpy(retrieved_data.data(), retrieved.data(),
                data.size() * sizeof(uint32_t));

    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(retrieved_data[i], data[i]);
    }
}

// ============================================================================
// BufferList Tests
// ============================================================================

TEST(YourBufferListTest, FirstAllocationStartsAtZero) {
    auto &gpu = vw::tests::create_gpu();

    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;
    StorageBufferList list(gpu.allocator);

    auto info = list.create_buffer(100);

    EXPECT_EQ(info.offset, 0);
    EXPECT_NE(info.buffer, nullptr);
}

TEST(YourBufferListTest, SequentialAllocations) {
    auto &gpu = vw::tests::create_gpu();

    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;
    StorageBufferList list(gpu.allocator);

    auto info1 = list.create_buffer(100);
    auto info2 = list.create_buffer(50);

    EXPECT_EQ(info1.offset, 0);
    EXPECT_EQ(info2.offset, 100);
    // Both should use the same underlying buffer
    EXPECT_EQ(info1.buffer, info2.buffer);
}

TEST(YourBufferListTest, AlignedAllocations) {
    auto &gpu = vw::tests::create_gpu();

    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;
    StorageBufferList list(gpu.allocator);

    auto info1 = list.create_buffer(100);       // 0-99
    auto info2 = list.create_buffer(50, 256);   // aligned to 256

    EXPECT_EQ(info1.offset, 0);
    EXPECT_EQ(info2.offset, 256);
    EXPECT_EQ(info2.offset % 256, 0);
}

TEST(YourBufferListTest, MultipleAlignedAllocations) {
    auto &gpu = vw::tests::create_gpu();

    using StorageBufferList =
        vw::BufferList<std::byte, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;
    StorageBufferList list(gpu.allocator);

    auto info1 = list.create_buffer(100, 256);  // 0-99
    auto info2 = list.create_buffer(200, 256);  // 256-455
    auto info3 = list.create_buffer(50, 256);   // 512-561

    EXPECT_EQ(info1.offset, 0);
    EXPECT_EQ(info2.offset, 256);
    EXPECT_EQ(info3.offset, 512);

    // All offsets should be 256-byte aligned
    EXPECT_EQ(info1.offset % 256, 0);
    EXPECT_EQ(info2.offset % 256, 0);
    EXPECT_EQ(info3.offset % 256, 0);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(YourBufferTest, CreateMinimalSizeBuffer) {
    auto &gpu = vw::tests::create_gpu();

    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, 1);

    EXPECT_EQ(buffer.size(), 1);
}

// Add tests for:
// - Maximum buffer sizes
// - Buffer address (for device address usage)
// - Transfer operations
// - Concurrent access patterns

} // namespace
