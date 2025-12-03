#include <gtest/gtest.h>
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include "utils/create_gpu.hpp"
#include <vector>

TEST(StagingBufferManagerTest, CreateStagingBufferManager) {
    auto& gpu = vw::tests::create_gpu();
    vw::StagingBufferManager staging_manager(*gpu.device, *gpu.allocator);
    SUCCEED();
}

TEST(StagingBufferManagerTest, TransferDataToDeviceBuffer) {
    auto& gpu = vw::tests::create_gpu();

    // Create StagingBufferManager
    vw::StagingBufferManager staging_manager(*gpu.device, *gpu.allocator);

    // Create a device-only buffer with transfer destination and source capability
    constexpr VkBufferUsageFlags DeviceBufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using DeviceBuffer = vw::Buffer<float, false, DeviceBufferUsage>;
    auto device_buffer = vw::create_buffer<DeviceBuffer>(*gpu.allocator, 10);

    // Prepare test data
    std::vector<float> test_data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};

    // Stage the data to the device buffer
    staging_manager.fill_buffer(std::span<const float>(test_data), device_buffer, 0);

    // Get the command buffer with transfer commands and submit it
    auto& queue = gpu.queue();
    auto transfer_cmd = staging_manager.fill_command_buffer();
    queue.enqueue_command_buffer(transfer_cmd);
    auto fence = queue.submit({}, {}, {});
    fence.wait();

    // Create a host-visible buffer to read back the data
    using HostBuffer = vw::Buffer<float, true, vw::StagingBufferUsage>;
    auto host_buffer = vw::create_buffer<HostBuffer>(*gpu.allocator, 10);

    // Create a command to copy from device to host
    auto cmd_pool = vw::CommandPoolBuilder(*gpu.device).build();
    auto readback_cmd = cmd_pool.allocate(1)[0];

    std::ignore = readback_cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::BufferCopy copy_region;
    copy_region.setSrcOffset(0)
               .setDstOffset(0)
               .setSize(test_data.size() * sizeof(float));

    readback_cmd.copyBuffer(device_buffer.handle(), host_buffer.handle(), copy_region);
    std::ignore = readback_cmd.end();

    // Submit readback command and wait
    queue.enqueue_command_buffer(readback_cmd);
    auto readback_fence = queue.submit({}, {}, {});
    readback_fence.wait();

    // Verify the data
    auto retrieved = host_buffer.as_vector(0, test_data.size());
    ASSERT_EQ(retrieved.size(), test_data.size());

    for (size_t i = 0; i < test_data.size(); ++i) {
        EXPECT_FLOAT_EQ(retrieved[i], test_data[i])
            << "Mismatch at index " << i;
    }
}

TEST(StagingBufferManagerTest, TransferIntegerData) {
    auto& gpu = vw::tests::create_gpu();
    vw::StagingBufferManager staging_manager(*gpu.device, *gpu.allocator);

    // Create a device-only buffer for integers
    constexpr VkBufferUsageFlags DeviceBufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using DeviceBuffer = vw::Buffer<int32_t, false, DeviceBufferUsage>;
    auto device_buffer = vw::create_buffer<DeviceBuffer>(*gpu.allocator, 20);

    // Prepare test data
    std::vector<int32_t> test_data;
    for (int32_t i = 0; i < 20; ++i) {
        test_data.push_back(i * 100);
    }

    // Stage the data
    staging_manager.fill_buffer(std::span<const int32_t>(test_data), device_buffer, 0);

    // Submit transfer commands
    auto& queue = gpu.queue();
    auto transfer_cmd = staging_manager.fill_command_buffer();
    queue.enqueue_command_buffer(transfer_cmd);
    auto fence = queue.submit({}, {}, {});
    fence.wait();

    // Read back and verify
    using HostBuffer = vw::Buffer<int32_t, true, vw::StagingBufferUsage>;
    auto host_buffer = vw::create_buffer<HostBuffer>(*gpu.allocator, 20);

    auto cmd_pool = vw::CommandPoolBuilder(*gpu.device).build();
    auto readback_cmd = cmd_pool.allocate(1)[0];

    std::ignore = readback_cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::BufferCopy copy_region;
    copy_region.setSrcOffset(0)
               .setDstOffset(0)
               .setSize(test_data.size() * sizeof(int32_t));

    readback_cmd.copyBuffer(device_buffer.handle(), host_buffer.handle(), copy_region);
    std::ignore = readback_cmd.end();

    queue.enqueue_command_buffer(readback_cmd);
    auto readback_fence = queue.submit({}, {}, {});
    readback_fence.wait();

    auto retrieved = host_buffer.as_vector(0, test_data.size());
    ASSERT_EQ(retrieved.size(), test_data.size());

    for (size_t i = 0; i < test_data.size(); ++i) {
        EXPECT_EQ(retrieved[i], test_data[i])
            << "Mismatch at index " << i;
    }
}

TEST(StagingBufferManagerTest, TransferDoubleData) {
    auto& gpu = vw::tests::create_gpu();
    vw::StagingBufferManager staging_manager(*gpu.device, *gpu.allocator);

    // Create a device-only buffer for doubles
    constexpr VkBufferUsageFlags DeviceBufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using DeviceBuffer = vw::Buffer<double, false, DeviceBufferUsage>;
    auto device_buffer = vw::create_buffer<DeviceBuffer>(*gpu.allocator, 15);

    // Prepare test data
    std::vector<double> test_data;
    for (int i = 0; i < 15; ++i) {
        test_data.push_back(i * 3.14159265358979323846);
    }

    // Stage the data
    staging_manager.fill_buffer(std::span<const double>(test_data), device_buffer, 0);

    // Submit transfer commands
    auto& queue = gpu.queue();
    auto transfer_cmd = staging_manager.fill_command_buffer();
    queue.enqueue_command_buffer(transfer_cmd);
    auto fence = queue.submit({}, {}, {});
    fence.wait();

    // Read back and verify
    using HostBuffer = vw::Buffer<double, true, vw::StagingBufferUsage>;
    auto host_buffer = vw::create_buffer<HostBuffer>(*gpu.allocator, 15);

    auto cmd_pool = vw::CommandPoolBuilder(*gpu.device).build();
    auto readback_cmd = cmd_pool.allocate(1)[0];

    std::ignore = readback_cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::BufferCopy copy_region;
    copy_region.setSrcOffset(0)
               .setDstOffset(0)
               .setSize(test_data.size() * sizeof(double));

    readback_cmd.copyBuffer(device_buffer.handle(), host_buffer.handle(), copy_region);
    std::ignore = readback_cmd.end();

    queue.enqueue_command_buffer(readback_cmd);
    auto readback_fence = queue.submit({}, {}, {});
    readback_fence.wait();

    auto retrieved = host_buffer.as_vector(0, test_data.size());
    ASSERT_EQ(retrieved.size(), test_data.size());

    for (size_t i = 0; i < test_data.size(); ++i) {
        EXPECT_DOUBLE_EQ(retrieved[i], test_data[i])
            << "Mismatch at index " << i;
    }
}

// Simple structure test
struct Vec3 {
    float x, y, z;

    bool operator==(const Vec3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

TEST(StagingBufferManagerTest, TransferSimpleStructData) {
    auto& gpu = vw::tests::create_gpu();
    vw::StagingBufferManager staging_manager(*gpu.device, *gpu.allocator);

    // Create a device-only buffer for Vec3 structures
    constexpr VkBufferUsageFlags DeviceBufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using DeviceBuffer = vw::Buffer<Vec3, false, DeviceBufferUsage>;
    auto device_buffer = vw::create_buffer<DeviceBuffer>(*gpu.allocator, 8);

    // Prepare test data
    std::vector<Vec3> test_data = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
        {10.0f, 11.0f, 12.0f},
        {13.0f, 14.0f, 15.0f},
        {16.0f, 17.0f, 18.0f},
        {19.0f, 20.0f, 21.0f},
        {22.0f, 23.0f, 24.0f}
    };

    // Stage the data
    staging_manager.fill_buffer(std::span<const Vec3>(test_data), device_buffer, 0);

    // Submit transfer commands
    auto& queue = gpu.queue();
    auto transfer_cmd = staging_manager.fill_command_buffer();
    queue.enqueue_command_buffer(transfer_cmd);
    auto fence = queue.submit({}, {}, {});
    fence.wait();

    // Read back and verify
    using HostBuffer = vw::Buffer<Vec3, true, vw::StagingBufferUsage>;
    auto host_buffer = vw::create_buffer<HostBuffer>(*gpu.allocator, 8);

    auto cmd_pool = vw::CommandPoolBuilder(*gpu.device).build();
    auto readback_cmd = cmd_pool.allocate(1)[0];

    std::ignore = readback_cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::BufferCopy copy_region;
    copy_region.setSrcOffset(0)
               .setDstOffset(0)
               .setSize(test_data.size() * sizeof(Vec3));

    readback_cmd.copyBuffer(device_buffer.handle(), host_buffer.handle(), copy_region);
    std::ignore = readback_cmd.end();

    queue.enqueue_command_buffer(readback_cmd);
    auto readback_fence = queue.submit({}, {}, {});
    readback_fence.wait();

    auto retrieved = host_buffer.as_vector(0, test_data.size());
    ASSERT_EQ(retrieved.size(), test_data.size());

    for (size_t i = 0; i < test_data.size(); ++i) {
        EXPECT_EQ(retrieved[i], test_data[i])
            << "Mismatch at index " << i;
    }
}

// Complex structure with different alignment requirements
struct alignas(16) ParticleData {
    float position[3];
    float mass;
    float velocity[3];
    uint32_t id;

    bool operator==(const ParticleData& other) const {
        return position[0] == other.position[0] &&
               position[1] == other.position[1] &&
               position[2] == other.position[2] &&
               mass == other.mass &&
               velocity[0] == other.velocity[0] &&
               velocity[1] == other.velocity[1] &&
               velocity[2] == other.velocity[2] &&
               id == other.id;
    }
};

TEST(StagingBufferManagerTest, TransferComplexStructData) {
    auto& gpu = vw::tests::create_gpu();
    vw::StagingBufferManager staging_manager(*gpu.device, *gpu.allocator);

    // Create a device-only buffer for ParticleData structures
    constexpr VkBufferUsageFlags DeviceBufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using DeviceBuffer = vw::Buffer<ParticleData, false, DeviceBufferUsage>;
    auto device_buffer = vw::create_buffer<DeviceBuffer>(*gpu.allocator, 5);

    // Prepare test data
    std::vector<ParticleData> test_data;
    for (uint32_t i = 0; i < 5; ++i) {
        ParticleData particle;
        particle.position[0] = static_cast<float>(i * 1.0f);
        particle.position[1] = static_cast<float>(i * 2.0f);
        particle.position[2] = static_cast<float>(i * 3.0f);
        particle.mass = static_cast<float>(i + 1) * 0.5f;
        particle.velocity[0] = static_cast<float>(i * 0.1f);
        particle.velocity[1] = static_cast<float>(i * 0.2f);
        particle.velocity[2] = static_cast<float>(i * 0.3f);
        particle.id = i * 100;
        test_data.push_back(particle);
    }

    // Stage the data
    staging_manager.fill_buffer(std::span<const ParticleData>(test_data), device_buffer, 0);

    // Submit transfer commands
    auto& queue = gpu.queue();
    auto transfer_cmd = staging_manager.fill_command_buffer();
    queue.enqueue_command_buffer(transfer_cmd);
    auto fence = queue.submit({}, {}, {});
    fence.wait();

    // Read back and verify
    using HostBuffer = vw::Buffer<ParticleData, true, vw::StagingBufferUsage>;
    auto host_buffer = vw::create_buffer<HostBuffer>(*gpu.allocator, 5);

    auto cmd_pool = vw::CommandPoolBuilder(*gpu.device).build();
    auto readback_cmd = cmd_pool.allocate(1)[0];

    std::ignore = readback_cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::BufferCopy copy_region;
    copy_region.setSrcOffset(0)
               .setDstOffset(0)
               .setSize(test_data.size() * sizeof(ParticleData));

    readback_cmd.copyBuffer(device_buffer.handle(), host_buffer.handle(), copy_region);
    std::ignore = readback_cmd.end();

    queue.enqueue_command_buffer(readback_cmd);
    auto readback_fence = queue.submit({}, {}, {});
    readback_fence.wait();

    auto retrieved = host_buffer.as_vector(0, test_data.size());
    ASSERT_EQ(retrieved.size(), test_data.size());

    for (size_t i = 0; i < test_data.size(); ++i) {
        EXPECT_EQ(retrieved[i], test_data[i])
            << "Mismatch at index " << i;
    }
}

TEST(StagingBufferManagerTest, TransferWithOffset) {
    auto& gpu = vw::tests::create_gpu();
    vw::StagingBufferManager staging_manager(*gpu.device, *gpu.allocator);

    // Create a device-only buffer
    constexpr VkBufferUsageFlags DeviceBufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using DeviceBuffer = vw::Buffer<float, false, DeviceBufferUsage>;
    auto device_buffer = vw::create_buffer<DeviceBuffer>(*gpu.allocator, 20);

    // First transfer: write to beginning
    std::vector<float> first_data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    staging_manager.fill_buffer(std::span<const float>(first_data), device_buffer, 0);

    auto& queue = gpu.queue();
    auto transfer_cmd1 = staging_manager.fill_command_buffer();
    queue.enqueue_command_buffer(transfer_cmd1);
    auto fence1 = queue.submit({}, {}, {});
    fence1.wait();

    // Second transfer: write with offset
    std::vector<float> second_data = {10.0f, 11.0f, 12.0f, 13.0f, 14.0f};
    staging_manager.fill_buffer(std::span<const float>(second_data), device_buffer, 10);

    auto transfer_cmd2 = staging_manager.fill_command_buffer();
    queue.enqueue_command_buffer(transfer_cmd2);
    auto fence2 = queue.submit({}, {}, {});
    fence2.wait();

    // Read back and verify
    using HostBuffer = vw::Buffer<float, true, vw::StagingBufferUsage>;
    auto host_buffer = vw::create_buffer<HostBuffer>(*gpu.allocator, 20);

    auto cmd_pool = vw::CommandPoolBuilder(*gpu.device).build();
    auto readback_cmd = cmd_pool.allocate(1)[0];

    std::ignore = readback_cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::BufferCopy copy_region;
    copy_region.setSrcOffset(0)
               .setDstOffset(0)
               .setSize(20 * sizeof(float));

    readback_cmd.copyBuffer(device_buffer.handle(), host_buffer.handle(), copy_region);
    std::ignore = readback_cmd.end();

    queue.enqueue_command_buffer(readback_cmd);
    auto readback_fence = queue.submit({}, {}, {});
    readback_fence.wait();

    auto retrieved = host_buffer.as_vector(0, 20);

    // Verify first transfer
    for (size_t i = 0; i < first_data.size(); ++i) {
        EXPECT_FLOAT_EQ(retrieved[i], first_data[i])
            << "First transfer mismatch at index " << i;
    }

    // Verify second transfer (at offset 10)
    for (size_t i = 0; i < second_data.size(); ++i) {
        EXPECT_FLOAT_EQ(retrieved[10 + i], second_data[i])
            << "Second transfer mismatch at index " << (10 + i);
    }
}

TEST(StagingBufferManagerTest, TransferMultipleSequential) {
    auto& gpu = vw::tests::create_gpu();
    vw::StagingBufferManager staging_manager(*gpu.device, *gpu.allocator);

    // Create three device buffers
    constexpr VkBufferUsageFlags DeviceBufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using DeviceBuffer = vw::Buffer<float, false, DeviceBufferUsage>;
    auto buffer1 = vw::create_buffer<DeviceBuffer>(*gpu.allocator, 5);
    auto buffer2 = vw::create_buffer<DeviceBuffer>(*gpu.allocator, 5);
    auto buffer3 = vw::create_buffer<DeviceBuffer>(*gpu.allocator, 5);

    // Prepare test data
    std::vector<float> data1 = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    std::vector<float> data2 = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f};
    std::vector<float> data3 = {100.0f, 200.0f, 300.0f, 400.0f, 500.0f};

    // Stage all data in one go
    staging_manager.fill_buffer(std::span<const float>(data1), buffer1, 0);
    staging_manager.fill_buffer(std::span<const float>(data2), buffer2, 0);
    staging_manager.fill_buffer(std::span<const float>(data3), buffer3, 0);

    // Submit all transfers at once
    auto& queue = gpu.queue();
    auto transfer_cmd = staging_manager.fill_command_buffer();
    queue.enqueue_command_buffer(transfer_cmd);
    auto fence = queue.submit({}, {}, {});
    fence.wait();

    // Read back and verify all buffers
    using HostBuffer = vw::Buffer<float, true, vw::StagingBufferUsage>;
    auto host_buffer1 = vw::create_buffer<HostBuffer>(*gpu.allocator, 5);
    auto host_buffer2 = vw::create_buffer<HostBuffer>(*gpu.allocator, 5);
    auto host_buffer3 = vw::create_buffer<HostBuffer>(*gpu.allocator, 5);

    auto cmd_pool = vw::CommandPoolBuilder(*gpu.device).build();
    auto readback_cmd = cmd_pool.allocate(1)[0];

    std::ignore = readback_cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::BufferCopy copy_region;
    copy_region.setSrcOffset(0)
               .setDstOffset(0)
               .setSize(5 * sizeof(float));

    readback_cmd.copyBuffer(buffer1.handle(), host_buffer1.handle(), copy_region);
    readback_cmd.copyBuffer(buffer2.handle(), host_buffer2.handle(), copy_region);
    readback_cmd.copyBuffer(buffer3.handle(), host_buffer3.handle(), copy_region);
    std::ignore = readback_cmd.end();

    queue.enqueue_command_buffer(readback_cmd);
    auto readback_fence = queue.submit({}, {}, {});
    readback_fence.wait();

    // Verify all buffers
    auto retrieved1 = host_buffer1.as_vector(0, 5);
    auto retrieved2 = host_buffer2.as_vector(0, 5);
    auto retrieved3 = host_buffer3.as_vector(0, 5);

    for (size_t i = 0; i < 5; ++i) {
        EXPECT_FLOAT_EQ(retrieved1[i], data1[i]) << "Buffer1 mismatch at index " << i;
        EXPECT_FLOAT_EQ(retrieved2[i], data2[i]) << "Buffer2 mismatch at index " << i;
        EXPECT_FLOAT_EQ(retrieved3[i], data3[i]) << "Buffer3 mismatch at index " << i;
    }
}

TEST(StagingBufferManagerTest, TransferLargeDataSet) {
    auto& gpu = vw::tests::create_gpu();
    vw::StagingBufferManager staging_manager(*gpu.device, *gpu.allocator);

    // Create a large device buffer (1 million floats = ~4MB)
    constexpr size_t element_count = 1'000'000;
    constexpr VkBufferUsageFlags DeviceBufferUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    using DeviceBuffer = vw::Buffer<float, false, DeviceBufferUsage>;
    auto device_buffer = vw::create_buffer<DeviceBuffer>(*gpu.allocator, element_count);

    // Prepare large test data
    std::vector<float> test_data;
    test_data.reserve(element_count);
    for (size_t i = 0; i < element_count; ++i) {
        test_data.push_back(static_cast<float>(i) * 0.001f);
    }

    // Stage the data
    staging_manager.fill_buffer(std::span<const float>(test_data), device_buffer, 0);

    // Submit transfer commands
    auto& queue = gpu.queue();
    auto transfer_cmd = staging_manager.fill_command_buffer();
    queue.enqueue_command_buffer(transfer_cmd);
    auto fence = queue.submit({}, {}, {});
    fence.wait();

    // Read back and verify (sample verification to keep test fast)
    using HostBuffer = vw::Buffer<float, true, vw::StagingBufferUsage>;
    auto host_buffer = vw::create_buffer<HostBuffer>(*gpu.allocator, element_count);

    auto cmd_pool = vw::CommandPoolBuilder(*gpu.device).build();
    auto readback_cmd = cmd_pool.allocate(1)[0];

    std::ignore = readback_cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::BufferCopy copy_region;
    copy_region.setSrcOffset(0)
               .setDstOffset(0)
               .setSize(element_count * sizeof(float));

    readback_cmd.copyBuffer(device_buffer.handle(), host_buffer.handle(), copy_region);
    std::ignore = readback_cmd.end();

    queue.enqueue_command_buffer(readback_cmd);
    auto readback_fence = queue.submit({}, {}, {});
    readback_fence.wait();

    auto retrieved = host_buffer.as_vector(0, element_count);
    ASSERT_EQ(retrieved.size(), test_data.size());

    // Verify first 100 elements
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_FLOAT_EQ(retrieved[i], test_data[i])
            << "Mismatch at beginning index " << i;
    }

    // Verify middle 100 elements
    size_t middle = element_count / 2;
    for (size_t i = middle; i < middle + 100; ++i) {
        EXPECT_FLOAT_EQ(retrieved[i], test_data[i])
            << "Mismatch at middle index " << i;
    }

    // Verify last 100 elements
    for (size_t i = element_count - 100; i < element_count; ++i) {
        EXPECT_FLOAT_EQ(retrieved[i], test_data[i])
            << "Mismatch at end index " << i;
    }
}

