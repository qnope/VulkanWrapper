#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include <gtest/gtest.h>

using namespace vw;
using namespace vw::Barrier;

namespace vw::Barrier {

class ResourceTrackerTest : public ::testing::Test {
  protected:
    ResourceTracker tracker;

    // Helper methods to access private members
    const std::vector<vk::BufferMemoryBarrier2> &
    getPendingBufferBarriers() const {
        return tracker.m_pending_buffer_barriers;
    }

    const std::vector<vk::ImageMemoryBarrier2> &
    getPendingImageBarriers() const {
        return tracker.m_pending_image_barriers;
    }

    const std::vector<vk::MemoryBarrier2> &getPendingMemoryBarriers() const {
        return tracker.m_pending_memory_barriers;
    }

    // Helper to inspect buffer state
    // Returns a list of (Interval, State) for a given buffer
    struct BufferStateInfo {
        BufferInterval interval;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
    };

    std::vector<BufferStateInfo> getBufferStates(vk::Buffer buffer) {
        std::vector<BufferStateInfo> result;
        if (tracker.m_buffer_states.find(buffer) ==
            tracker.m_buffer_states.end()) {
            return result;
        }

        const auto &stateSets = tracker.m_buffer_states[buffer];
        for (const auto &stateSet : stateSets) {
            for (const auto &interval : stateSet.intervals.intervals()) {
                result.push_back(
                    {interval, stateSet.state.stage, stateSet.state.access});
            }
        }
        // Sort by offset for easier testing
        std::sort(result.begin(), result.end(),
                  [](const BufferStateInfo &a, const BufferStateInfo &b) {
                      return a.interval.offset < b.interval.offset;
                  });
        return result;
    }

    // Helper to inspect image state
    struct ImageStateInfo {
        ImageInterval interval;
        vk::ImageLayout layout;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
    };

    std::vector<ImageStateInfo> getImageStates(vk::Image image) {
        std::vector<ImageStateInfo> result;
        if (tracker.m_image_states.find(image) ==
            tracker.m_image_states.end()) {
            return result;
        }

        const auto &stateSets = tracker.m_image_states[image];
        for (const auto &stateSet : stateSets) {
            for (const auto &interval : stateSet.intervals.intervals()) {
                result.push_back({interval, stateSet.state.layout,
                                  stateSet.state.stage, stateSet.state.access});
            }
        }
        // No simple sort for multi-dimensional image intervals, but we can rely
        // on insertion order or just check existence
        return result;
    }

    void clearPendingBarriers() {
        tracker.m_pending_buffer_barriers.clear();
        tracker.m_pending_image_barriers.clear();
        tracker.m_pending_memory_barriers.clear();
    }
};

// =================================================================================================
// Buffer Tests
// =================================================================================================

TEST_F(ResourceTrackerTest, Buffer_UntrackedRequest_GeneratesFullBarrier) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    // Request a buffer range that hasn't been tracked yet
    tracker.request(BufferState{.buffer = buffer,
                                .offset = 0,
                                .size = 1024,
                                .stage = vk::PipelineStageFlagBits2::eTransfer,
                                .access = vk::AccessFlagBits2::eTransferWrite});

    // Should have full memory barrier
    auto barriers = getPendingBufferBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].srcStageMask,
              vk::PipelineStageFlagBits2::eAllCommands);
    EXPECT_EQ(barriers[0].srcAccessMask, vk::AccessFlagBits2::eMemoryWrite |
                                             vk::AccessFlagBits2::eMemoryRead);
    EXPECT_EQ(barriers[0].dstStageMask, vk::PipelineStageFlagBits2::eTransfer);
    EXPECT_EQ(barriers[0].dstAccessMask, vk::AccessFlagBits2::eTransferWrite);

    // Should have state tracked
    auto states = getBufferStates(buffer);
    ASSERT_EQ(states.size(), 1);
    EXPECT_EQ(states[0].interval.offset, 0);
    EXPECT_EQ(states[0].interval.size, 1024);
    EXPECT_EQ(states[0].stage, vk::PipelineStageFlagBits2::eTransfer);
    EXPECT_EQ(states[0].access, vk::AccessFlagBits2::eTransferWrite);
}

TEST_F(ResourceTrackerTest, Buffer_RAR_NoBarrier) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    // Initial state: Shader Read
    tracker.track(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 1024,
                    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    // Request: Vertex Shader Read (Read After Read)
    tracker.request(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 1024,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    // RAR should not generate a barrier
    EXPECT_TRUE(getPendingBufferBarriers().empty());

    // State should be updated (or merged/kept? Actually request updates state
    // to new usage) Wait, if we request new state, the tracker updates the
    // state to the NEW usage. So subsequent barriers will transition FROM this
    // new state.
    auto states = getBufferStates(buffer);
    ASSERT_EQ(states.size(), 1);
    EXPECT_EQ(states[0].stage, vk::PipelineStageFlagBits2::eVertexShader);
    EXPECT_EQ(states[0].access, vk::AccessFlagBits2::eShaderRead);
}

TEST_F(ResourceTrackerTest, Buffer_RAW_GeneratesBarrier) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    // Initial: Write
    tracker.track(BufferState{.buffer = buffer,
                              .offset = 0,
                              .size = 1024,
                              .stage = vk::PipelineStageFlagBits2::eTransfer,
                              .access = vk::AccessFlagBits2::eTransferWrite});

    // Request: Read (Read After Write)
    tracker.request(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 1024,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    // Should generate barrier
    auto barriers = getPendingBufferBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].srcStageMask, vk::PipelineStageFlagBits2::eTransfer);
    EXPECT_EQ(barriers[0].srcAccessMask, vk::AccessFlagBits2::eTransferWrite);
    EXPECT_EQ(barriers[0].dstStageMask,
              vk::PipelineStageFlagBits2::eVertexShader);
    EXPECT_EQ(barriers[0].dstAccessMask, vk::AccessFlagBits2::eShaderRead);
    EXPECT_EQ(barriers[0].offset, 0);
    EXPECT_EQ(barriers[0].size, 1024);

    // State updated
    auto states = getBufferStates(buffer);
    ASSERT_EQ(states.size(), 1);
    EXPECT_EQ(states[0].access, vk::AccessFlagBits2::eShaderRead);
}

TEST_F(ResourceTrackerTest, Buffer_WAR_GeneratesBarrier) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    // Initial: Read
    tracker.track(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 1024,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    // Request: Write (Write After Read)
    tracker.request(BufferState{.buffer = buffer,
                                .offset = 0,
                                .size = 1024,
                                .stage = vk::PipelineStageFlagBits2::eTransfer,
                                .access = vk::AccessFlagBits2::eTransferWrite});

    auto barriers = getPendingBufferBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].srcAccessMask, vk::AccessFlagBits2::eShaderRead);
    EXPECT_EQ(barriers[0].dstAccessMask, vk::AccessFlagBits2::eTransferWrite);
}

TEST_F(ResourceTrackerTest, Buffer_WAW_GeneratesBarrier) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    // Initial: Write
    tracker.track(BufferState{.buffer = buffer,
                              .offset = 0,
                              .size = 1024,
                              .stage = vk::PipelineStageFlagBits2::eTransfer,
                              .access = vk::AccessFlagBits2::eTransferWrite});

    // Request: Write (Write After Write)
    tracker.request(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 1024,
                    .stage = vk::PipelineStageFlagBits2::eComputeShader,
                    .access = vk::AccessFlagBits2::eShaderWrite});

    auto barriers = getPendingBufferBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].srcAccessMask, vk::AccessFlagBits2::eTransferWrite);
    EXPECT_EQ(barriers[0].dstAccessMask, vk::AccessFlagBits2::eShaderWrite);
}

TEST_F(ResourceTrackerTest, Buffer_PartialOverlap_SplitsState) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    // Initial: [0, 1024] Write
    tracker.track(BufferState{.buffer = buffer,
                              .offset = 0,
                              .size = 1024,
                              .stage = vk::PipelineStageFlagBits2::eTransfer,
                              .access = vk::AccessFlagBits2::eTransferWrite});

    // Request: [0, 512] Read
    tracker.request(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 512,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    // Barrier should be only for [0, 512]
    auto barriers = getPendingBufferBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].offset, 0);
    EXPECT_EQ(barriers[0].size, 512);

    // States should be split:
    // [0, 512] -> Read
    // [512, 512] -> Write (Original state preserved for non-overlapping part)
    auto states = getBufferStates(buffer);
    ASSERT_EQ(states.size(), 2);

    // Check first interval [0, 512]
    EXPECT_EQ(states[0].interval.offset, 0);
    EXPECT_EQ(states[0].interval.size, 512);
    EXPECT_EQ(states[0].access, vk::AccessFlagBits2::eShaderRead);

    // Check second interval [512, 512] (offset 512, size 512)
    EXPECT_EQ(states[1].interval.offset, 512);
    EXPECT_EQ(states[1].interval.size, 512);
    EXPECT_EQ(states[1].access, vk::AccessFlagBits2::eTransferWrite);
}

TEST_F(ResourceTrackerTest, Buffer_MergeStates) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    // [0, 512] -> Read
    tracker.track(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 512,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    // [512, 512] -> Read (Adjacent, same state)
    tracker.track(
        BufferState{.buffer = buffer,
                    .offset = 512,
                    .size = 512,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    // Should merge into single [0, 1024] Read
    auto states = getBufferStates(buffer);
    ASSERT_EQ(states.size(), 1);
    EXPECT_EQ(states[0].interval.offset, 0);
    EXPECT_EQ(states[0].interval.size, 1024);
    EXPECT_EQ(states[0].access, vk::AccessFlagBits2::eShaderRead);
}

// =================================================================================================
// Image Tests
// =================================================================================================

TEST_F(ResourceTrackerTest, Image_UntrackedRequest_AddsState) {
    vk::Image image = vk::Image(reinterpret_cast<VkImage>(0x200));
    vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor, 0, 1, 0,
                                    1};

    tracker.request(
        ImageState{.image = image,
                   .subresourceRange = range,
                   .layout = vk::ImageLayout::eColorAttachmentOptimal,
                   .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                   .access = vk::AccessFlagBits2::eColorAttachmentWrite});

    // Should have barrier from Undefined -> ColorAttachmentOptimal
    auto barriers = getPendingImageBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].oldLayout, vk::ImageLayout::eUndefined);
    EXPECT_EQ(barriers[0].newLayout, vk::ImageLayout::eColorAttachmentOptimal);
    EXPECT_EQ(barriers[0].srcStageMask, vk::PipelineStageFlagBits2::eNone);
    EXPECT_EQ(barriers[0].srcAccessMask, vk::AccessFlagBits2::eNone);
    EXPECT_EQ(barriers[0].dstStageMask,
              vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    EXPECT_EQ(barriers[0].dstAccessMask,
              vk::AccessFlagBits2::eColorAttachmentWrite);

    auto states = getImageStates(image);
    ASSERT_EQ(states.size(), 1);
    EXPECT_EQ(states[0].layout, vk::ImageLayout::eColorAttachmentOptimal);
}

TEST_F(ResourceTrackerTest, Image_LayoutTransition_GeneratesBarrier) {
    vk::Image image = vk::Image(reinterpret_cast<VkImage>(0x200));
    vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor, 0, 1, 0,
                                    1};

    // Initial: Undefined
    tracker.track(ImageState{.image = image,
                             .subresourceRange = range,
                             .layout = vk::ImageLayout::eUndefined,
                             .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
                             .access = vk::AccessFlagBits2::eNone});

    // Request: Transfer Dst
    tracker.request(ImageState{.image = image,
                               .subresourceRange = range,
                               .layout = vk::ImageLayout::eTransferDstOptimal,
                               .stage = vk::PipelineStageFlagBits2::eTransfer,
                               .access = vk::AccessFlagBits2::eTransferWrite});

    auto barriers = getPendingImageBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].oldLayout, vk::ImageLayout::eUndefined);
    EXPECT_EQ(barriers[0].newLayout, vk::ImageLayout::eTransferDstOptimal);
    EXPECT_EQ(barriers[0].srcStageMask, vk::PipelineStageFlagBits2::eTopOfPipe);
    EXPECT_EQ(barriers[0].dstStageMask, vk::PipelineStageFlagBits2::eTransfer);

    auto states = getImageStates(image);
    ASSERT_EQ(states.size(), 1);
    EXPECT_EQ(states[0].layout, vk::ImageLayout::eTransferDstOptimal);
}

TEST_F(ResourceTrackerTest, Image_SameLayout_DifferentAccess_GeneratesBarrier) {
    vk::Image image = vk::Image(reinterpret_cast<VkImage>(0x200));
    vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor, 0, 1, 0,
                                    1};

    // Initial: Color Attachment Write
    tracker.track(
        ImageState{.image = image,
                   .subresourceRange = range,
                   .layout = vk::ImageLayout::eColorAttachmentOptimal,
                   .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                   .access = vk::AccessFlagBits2::eColorAttachmentWrite});

    // Request: Fragment Shader Read (Same Layout)
    tracker.request(ImageState{
        .image = image,
        .subresourceRange = range,
        .layout =
            vk::ImageLayout::eColorAttachmentOptimal, // Keeping same layout for
                                                      // read (e.g. input
                                                      // attachment)
        .stage = vk::PipelineStageFlagBits2::eFragmentShader,
        .access = vk::AccessFlagBits2::eInputAttachmentRead});

    auto barriers = getPendingImageBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].oldLayout, vk::ImageLayout::eColorAttachmentOptimal);
    EXPECT_EQ(barriers[0].newLayout, vk::ImageLayout::eColorAttachmentOptimal);
    EXPECT_EQ(barriers[0].srcAccessMask,
              vk::AccessFlagBits2::eColorAttachmentWrite);
    EXPECT_EQ(barriers[0].dstAccessMask,
              vk::AccessFlagBits2::eInputAttachmentRead);
}

TEST_F(ResourceTrackerTest, Image_SubresourceOverlap_SplitsState) {
    vk::Image image = vk::Image(reinterpret_cast<VkImage>(0x200));
    // Range: Mip 0-2
    vk::ImageSubresourceRange fullRange{vk::ImageAspectFlagBits::eColor, 0, 3,
                                        0, 1};

    tracker.track(ImageState{.image = image,
                             .subresourceRange = fullRange,
                             .layout = vk::ImageLayout::eTransferDstOptimal,
                             .stage = vk::PipelineStageFlagBits2::eTransfer,
                             .access = vk::AccessFlagBits2::eTransferWrite});

    // Request Mip 1 only: Shader Read
    vk::ImageSubresourceRange mip1Range{vk::ImageAspectFlagBits::eColor, 1, 1,
                                        0, 1};
    tracker.request(
        ImageState{.image = image,
                   .subresourceRange = mip1Range,
                   .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
                   .stage = vk::PipelineStageFlagBits2::eFragmentShader,
                   .access = vk::AccessFlagBits2::eShaderRead});

    auto barriers = getPendingImageBarriers();
    ASSERT_EQ(barriers.size(), 1);
    // Barrier should be for Mip 1
    EXPECT_EQ(barriers[0].subresourceRange.baseMipLevel, 1);
    EXPECT_EQ(barriers[0].subresourceRange.levelCount, 1);
    EXPECT_EQ(barriers[0].newLayout, vk::ImageLayout::eShaderReadOnlyOptimal);

    // States should be split:
    // Mip 0: TransferDst
    // Mip 1: ShaderRead
    // Mip 2: TransferDst
    auto states = getImageStates(image);
    // Note: IntervalSet might merge Mip 0 and Mip 2 if they are not adjacent in
    // the set logic? Actually Mip 0 and Mip 2 are NOT adjacent (Mip 1 is in
    // between). So we expect 3 intervals if we iterate them. However, they
    // share the same state object in our implementation
    // (vector<pair<IntervalSet, State>>). So we might have 2 entries in the
    // vector:
    // 1. State=TransferDst, Intervals={Mip0, Mip2}
    // 2. State=ShaderRead, Intervals={Mip1}

    // Let's verify we have coverage for all mips
    bool hasMip0 = false;
    bool hasMip1 = false;
    bool hasMip2 = false;

    for (const auto &s : states) {
        if (s.interval.range.baseMipLevel == 0 &&
            s.interval.range.levelCount == 1 &&
            s.layout == vk::ImageLayout::eTransferDstOptimal)
            hasMip0 = true;
        if (s.interval.range.baseMipLevel == 1 &&
            s.interval.range.levelCount == 1 &&
            s.layout == vk::ImageLayout::eShaderReadOnlyOptimal)
            hasMip1 = true;
        if (s.interval.range.baseMipLevel == 2 &&
            s.interval.range.levelCount == 1 &&
            s.layout == vk::ImageLayout::eTransferDstOptimal)
            hasMip2 = true;
    }

    EXPECT_TRUE(hasMip0);
    EXPECT_TRUE(hasMip1);
    EXPECT_TRUE(hasMip2);
}

TEST_F(ResourceTrackerTest, Image_PartialUntracked_GeneratesBarriers) {
    vk::Image image = vk::Image(reinterpret_cast<VkImage>(0x200));

    // Track Mip 0
    vk::ImageSubresourceRange mip0Range{vk::ImageAspectFlagBits::eColor, 0, 1,
                                        0, 1};
    tracker.track(ImageState{.image = image,
                             .subresourceRange = mip0Range,
                             .layout = vk::ImageLayout::eTransferDstOptimal,
                             .stage = vk::PipelineStageFlagBits2::eTransfer,
                             .access = vk::AccessFlagBits2::eTransferWrite});

    // Request Mip 0 and Mip 1 (Mip 1 is untracked)
    vk::ImageSubresourceRange requestRange{vk::ImageAspectFlagBits::eColor, 0,
                                           2, 0, 1};
    tracker.request(
        ImageState{.image = image,
                   .subresourceRange = requestRange,
                   .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
                   .stage = vk::PipelineStageFlagBits2::eFragmentShader,
                   .access = vk::AccessFlagBits2::eShaderRead});

    auto barriers = getPendingImageBarriers();
    // Expect 2 barriers:
    // 1. Mip 0: TransferDst -> ShaderRead
    // 2. Mip 1: Undefined -> ShaderRead
    ASSERT_EQ(barriers.size(), 2);

    bool foundMip0 = false;
    bool foundMip1 = false;

    for (const auto &b : barriers) {
        if (b.subresourceRange.baseMipLevel == 0) {
            EXPECT_EQ(b.oldLayout, vk::ImageLayout::eTransferDstOptimal);
            foundMip0 = true;
        } else if (b.subresourceRange.baseMipLevel == 1) {
            EXPECT_EQ(b.oldLayout, vk::ImageLayout::eUndefined);
            foundMip1 = true;
        }
    }

    EXPECT_TRUE(foundMip0);
    EXPECT_TRUE(foundMip1);
}

TEST_F(ResourceTrackerTest, Image_UntrackedArrayLayer_GeneratesBarrier) {
    vk::Image image = vk::Image(reinterpret_cast<VkImage>(0x200));

    // Request Layer 2 (untracked)
    vk::ImageSubresourceRange requestRange{vk::ImageAspectFlagBits::eColor, 0,
                                           1, 2, 1};
    tracker.request(ImageState{.image = image,
                               .subresourceRange = requestRange,
                               .layout = vk::ImageLayout::eTransferDstOptimal,
                               .stage = vk::PipelineStageFlagBits2::eTransfer,
                               .access = vk::AccessFlagBits2::eTransferWrite});

    auto barriers = getPendingImageBarriers();
    ASSERT_EQ(barriers.size(), 1);

    // Barrier should be for Layer 2: Undefined -> TransferDst
    EXPECT_EQ(barriers[0].subresourceRange.baseArrayLayer, 2);
    EXPECT_EQ(barriers[0].subresourceRange.layerCount, 1);
    EXPECT_EQ(barriers[0].oldLayout, vk::ImageLayout::eUndefined);
    EXPECT_EQ(barriers[0].newLayout, vk::ImageLayout::eTransferDstOptimal);
    EXPECT_EQ(barriers[0].srcStageMask, vk::PipelineStageFlagBits2::eNone);
    EXPECT_EQ(barriers[0].srcAccessMask, vk::AccessFlagBits2::eNone);
    EXPECT_EQ(barriers[0].dstStageMask, vk::PipelineStageFlagBits2::eTransfer);
    EXPECT_EQ(barriers[0].dstAccessMask, vk::AccessFlagBits2::eTransferWrite);
}

} // namespace vw::Barrier

// =================================================================================================
// Acceleration Structure Tests
// =================================================================================================

TEST_F(ResourceTrackerTest, AS_UntrackedRequest_GeneratesFullBarrier) {
    vk::AccelerationStructureKHR as = vk::AccelerationStructureKHR(
        reinterpret_cast<VkAccelerationStructureKHR>(0x300));

    tracker.request(AccelerationStructureState{
        .handle = as,
        .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
        .access = vk::AccessFlagBits2::eAccelerationStructureWriteKHR});

    // Should have full memory barrier for untracked AS
    auto barriers = getPendingMemoryBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].srcStageMask,
              vk::PipelineStageFlagBits2::eAllCommands);
    EXPECT_EQ(barriers[0].srcAccessMask, vk::AccessFlagBits2::eMemoryWrite |
                                             vk::AccessFlagBits2::eMemoryRead);
    EXPECT_EQ(barriers[0].dstStageMask,
              vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR);
    EXPECT_EQ(barriers[0].dstAccessMask,
              vk::AccessFlagBits2::eAccelerationStructureWriteKHR);
}

TEST_F(ResourceTrackerTest, AS_Build_GeneratesBarrier) {
    vk::AccelerationStructureKHR as = vk::AccelerationStructureKHR(
        reinterpret_cast<VkAccelerationStructureKHR>(0x300));

    // Initial: Build (Write)
    tracker.track(AccelerationStructureState{
        .handle = as,
        .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
        .access = vk::AccessFlagBits2::eAccelerationStructureWriteKHR});

    // Request: Trace Rays (Read) -> Read After Write (RAW)
    tracker.request(AccelerationStructureState{
        .handle = as,
        .stage = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .access = vk::AccessFlagBits2::eAccelerationStructureReadKHR});

    auto barriers = getPendingMemoryBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].srcStageMask,
              vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR);
    EXPECT_EQ(barriers[0].srcAccessMask,
              vk::AccessFlagBits2::eAccelerationStructureWriteKHR);
    EXPECT_EQ(barriers[0].dstStageMask,
              vk::PipelineStageFlagBits2::eRayTracingShaderKHR);
    EXPECT_EQ(barriers[0].dstAccessMask,
              vk::AccessFlagBits2::eAccelerationStructureReadKHR);
}

TEST_F(ResourceTrackerTest, AS_Update_GeneratesBarrier) {
    vk::AccelerationStructureKHR as = vk::AccelerationStructureKHR(
        reinterpret_cast<VkAccelerationStructureKHR>(0x300));

    // Initial: Build (Write)
    tracker.track(AccelerationStructureState{
        .handle = as,
        .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
        .access = vk::AccessFlagBits2::eAccelerationStructureWriteKHR});

    // Request: Update (Write) -> Write After Write (WAW)
    tracker.request(AccelerationStructureState{
        .handle = as,
        .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
        .access = vk::AccessFlagBits2::eAccelerationStructureWriteKHR});

    auto barriers = getPendingMemoryBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].srcStageMask,
              vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR);
    EXPECT_EQ(barriers[0].srcAccessMask,
              vk::AccessFlagBits2::eAccelerationStructureWriteKHR);
    EXPECT_EQ(barriers[0].dstStageMask,
              vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR);
    EXPECT_EQ(barriers[0].dstAccessMask,
              vk::AccessFlagBits2::eAccelerationStructureWriteKHR);
}
