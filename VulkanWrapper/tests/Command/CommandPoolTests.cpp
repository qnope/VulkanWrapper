#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Command/CommandBuffer.h"
#include "VulkanWrapper/Command/CommandPool.h"
#include <gtest/gtest.h>

// CommandPoolBuilder Tests

TEST(CommandPoolBuilderTest, BuildCreatesValidCommandPool) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();

    EXPECT_NE(pool.handle(), nullptr);
}

// CommandPool Tests

TEST(CommandPoolTest, AllocateSingleCommandBuffer) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();

    auto buffers = pool.allocate(1);

    ASSERT_EQ(buffers.size(), 1);
    EXPECT_NE(buffers[0], nullptr);
}

TEST(CommandPoolTest, AllocateMultipleCommandBuffers) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();

    auto buffers = pool.allocate(5);

    ASSERT_EQ(buffers.size(), 5);
    for (const auto &buffer : buffers) {
        EXPECT_NE(buffer, nullptr);
    }
}

TEST(CommandPoolTest, AllocateLargeNumberOfCommandBuffers) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();

    auto buffers = pool.allocate(100);

    ASSERT_EQ(buffers.size(), 100);
    for (const auto &buffer : buffers) {
        EXPECT_NE(buffer, nullptr);
    }
}

TEST(CommandPoolTest, MultipleAllocationsFromSamePool) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();

    auto buffers1 = pool.allocate(3);
    auto buffers2 = pool.allocate(2);

    ASSERT_EQ(buffers1.size(), 3);
    ASSERT_EQ(buffers2.size(), 2);

    // All buffers should be valid
    for (const auto &buffer : buffers1) {
        EXPECT_NE(buffer, nullptr);
    }
    for (const auto &buffer : buffers2) {
        EXPECT_NE(buffer, nullptr);
    }
}

TEST(CommandPoolTest, AllocatedBuffersArePrimaryLevel) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();

    auto buffers = pool.allocate(1);

    // If we can begin the buffer with primary level info, it's primary level
    vk::CommandBufferBeginInfo beginInfo{};
    EXPECT_NO_THROW(buffers[0].begin(beginInfo));
    buffers[0].end();
}

// CommandBufferRecorder Tests

TEST(CommandBufferRecorderTest, RAIIBeginsAndEndsRecording) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();
    auto buffers = pool.allocate(1);

    {
        // Constructor should begin recording
        vw::CommandBufferRecorder recorder(buffers[0]);
        // Destructor will end recording
    }

    // Buffer should be in executable state after recorder destruction
    // We can verify by submitting to queue (would throw if not executable)
    gpu.queue().enqueue_command_buffer(buffers[0]);
    auto fence = gpu.queue().submit({}, {}, {});
    fence.wait();
}

TEST(CommandBufferRecorderTest, NonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<vw::CommandBufferRecorder>);
    EXPECT_FALSE(std::is_copy_assignable_v<vw::CommandBufferRecorder>);
}

TEST(CommandBufferRecorderTest, NonMovable) {
    EXPECT_FALSE(std::is_move_constructible_v<vw::CommandBufferRecorder>);
    EXPECT_FALSE(std::is_move_assignable_v<vw::CommandBufferRecorder>);
}

TEST(CommandBufferRecorderTest, MultipleRecordingsOnDifferentBuffers) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();
    auto buffers = pool.allocate(3);

    for (auto &buffer : buffers) {
        vw::CommandBufferRecorder recorder(buffer);
        // Each buffer records and ends properly
    }

    // All buffers should be executable
    gpu.queue().enqueue_command_buffers(buffers);
    auto fence = gpu.queue().submit({}, {}, {});
    fence.wait();
}

TEST(CommandBufferRecorderTest, RecordAndSubmitEmptyBuffer) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();
    auto buffers = pool.allocate(1);

    {
        vw::CommandBufferRecorder recorder(buffers[0]);
        // Empty recording - just begin and end
    }

    gpu.queue().enqueue_command_buffer(buffers[0]);
    auto fence = gpu.queue().submit({}, {}, {});
    fence.wait();

    SUCCEED();
}
