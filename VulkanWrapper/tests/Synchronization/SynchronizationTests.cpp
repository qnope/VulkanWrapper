#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Synchronization/Semaphore.h"
#include "VulkanWrapper/Command/CommandPool.h"
#include <gtest/gtest.h>

// Semaphore Tests

TEST(SemaphoreBuilderTest, BuildCreatesSemaphore) {
    auto &gpu = vw::tests::create_gpu();
    auto semaphore = vw::SemaphoreBuilder(gpu.device).build();

    EXPECT_NE(semaphore.handle(), nullptr);
}

TEST(SemaphoreTest, MultipleSemaphores) {
    auto &gpu = vw::tests::create_gpu();
    auto semaphore1 = vw::SemaphoreBuilder(gpu.device).build();
    auto semaphore2 = vw::SemaphoreBuilder(gpu.device).build();
    auto semaphore3 = vw::SemaphoreBuilder(gpu.device).build();

    EXPECT_NE(semaphore1.handle(), nullptr);
    EXPECT_NE(semaphore2.handle(), nullptr);
    EXPECT_NE(semaphore3.handle(), nullptr);
    EXPECT_NE(semaphore1.handle(), semaphore2.handle());
    EXPECT_NE(semaphore2.handle(), semaphore3.handle());
}

TEST(SemaphoreTest, SemaphoreIsMovable) {
    auto &gpu = vw::tests::create_gpu();
    auto semaphore1 = vw::SemaphoreBuilder(gpu.device).build();
    auto originalHandle = semaphore1.handle();

    auto semaphore2 = std::move(semaphore1);

    EXPECT_EQ(semaphore2.handle(), originalHandle);
}

// Fence Tests (using queue submission to create fences)

TEST(FenceTest, FenceFromQueueSubmit) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();
    auto buffers = pool.allocate(1);

    vk::CommandBufferBeginInfo beginInfo{};
    buffers[0].begin(beginInfo);
    buffers[0].end();

    gpu.queue().enqueue_command_buffer(buffers[0]);
    auto fence = gpu.queue().submit({}, {}, {});

    EXPECT_NE(fence.handle(), nullptr);
    fence.wait();
}

TEST(FenceTest, FenceWaitAndReset) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();
    auto buffers = pool.allocate(1);

    vk::CommandBufferBeginInfo beginInfo{};
    buffers[0].begin(beginInfo);
    buffers[0].end();

    gpu.queue().enqueue_command_buffer(buffers[0]);
    auto fence = gpu.queue().submit({}, {}, {});

    fence.wait();
    fence.reset();

    // After reset, we can use the fence again
    EXPECT_NE(fence.handle(), nullptr);
}

TEST(FenceTest, MultipleFences) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();
    auto buffers = pool.allocate(3);

    for (auto &buffer : buffers) {
        vk::CommandBufferBeginInfo beginInfo{};
        buffer.begin(beginInfo);
        buffer.end();
    }

    std::vector<vw::Fence> fences;
    for (int i = 0; i < 3; ++i) {
        gpu.queue().enqueue_command_buffer(buffers[i]);
        fences.push_back(gpu.queue().submit({}, {}, {}));
    }

    // Wait for all fences
    for (auto &fence : fences) {
        fence.wait();
    }

    SUCCEED();
}

TEST(FenceTest, FenceIsNonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<vw::Fence>);
    EXPECT_FALSE(std::is_copy_assignable_v<vw::Fence>);
}

TEST(FenceTest, FenceIsMovable) {
    EXPECT_TRUE(std::is_move_constructible_v<vw::Fence>);
    EXPECT_TRUE(std::is_move_assignable_v<vw::Fence>);
}

TEST(FenceTest, FenceMoveSemantics) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();
    auto buffers = pool.allocate(1);

    vk::CommandBufferBeginInfo beginInfo{};
    buffers[0].begin(beginInfo);
    buffers[0].end();

    gpu.queue().enqueue_command_buffer(buffers[0]);
    auto fence1 = gpu.queue().submit({}, {}, {});
    auto originalHandle = fence1.handle();

    auto fence2 = std::move(fence1);

    EXPECT_EQ(fence2.handle(), originalHandle);
    fence2.wait();
}

// Integration tests with semaphores

TEST(SynchronizationIntegrationTest, SubmitWithSignalSemaphore) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();
    auto buffers = pool.allocate(1);
    auto semaphore = vw::SemaphoreBuilder(gpu.device).build();

    vk::CommandBufferBeginInfo beginInfo{};
    buffers[0].begin(beginInfo);
    buffers[0].end();

    std::array<vk::Semaphore, 1> signalSemaphores = {semaphore.handle()};

    gpu.queue().enqueue_command_buffer(buffers[0]);
    auto fence = gpu.queue().submit({}, {}, signalSemaphores);
    fence.wait();

    SUCCEED();
}

TEST(SynchronizationIntegrationTest, ChainedSubmitsWithSemaphores) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();
    auto buffers = pool.allocate(2);
    auto semaphore = vw::SemaphoreBuilder(gpu.device).build();

    // Record first command buffer
    {
        vk::CommandBufferBeginInfo beginInfo{};
        buffers[0].begin(beginInfo);
        buffers[0].end();
    }

    // Record second command buffer
    {
        vk::CommandBufferBeginInfo beginInfo{};
        buffers[1].begin(beginInfo);
        buffers[1].end();
    }

    // First submit signals semaphore
    std::array<vk::Semaphore, 1> signalSemaphores = {semaphore.handle()};
    gpu.queue().enqueue_command_buffer(buffers[0]);
    auto fence1 = gpu.queue().submit({}, {}, signalSemaphores);

    // Second submit waits on semaphore
    std::array<vk::Semaphore, 1> waitSemaphores = {semaphore.handle()};
    std::array<vk::PipelineStageFlags, 1> waitStages = {
        vk::PipelineStageFlagBits::eTopOfPipe};
    gpu.queue().enqueue_command_buffer(buffers[1]);
    auto fence2 = gpu.queue().submit(waitStages, waitSemaphores, {});

    fence1.wait();
    fence2.wait();

    SUCCEED();
}

TEST(SynchronizationIntegrationTest, MultipleSequentialSubmits) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();
    auto buffers = pool.allocate(5);

    for (auto &buffer : buffers) {
        vk::CommandBufferBeginInfo beginInfo{};
        buffer.begin(beginInfo);
        buffer.end();
    }

    for (int i = 0; i < 5; ++i) {
        gpu.queue().enqueue_command_buffer(buffers[i]);
        auto fence = gpu.queue().submit({}, {}, {});
        fence.wait();
    }

    SUCCEED();
}

TEST(SynchronizationIntegrationTest, BatchSubmitAllBuffers) {
    auto &gpu = vw::tests::create_gpu();
    auto pool = vw::CommandPoolBuilder(gpu.device).build();
    auto buffers = pool.allocate(10);

    for (auto &buffer : buffers) {
        vk::CommandBufferBeginInfo beginInfo{};
        buffer.begin(beginInfo);
        buffer.end();
    }

    // Submit all at once
    gpu.queue().enqueue_command_buffers(buffers);
    auto fence = gpu.queue().submit({}, {}, {});
    fence.wait();

    SUCCEED();
}
