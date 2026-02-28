#include <gtest/gtest.h>
import vw;

using namespace vw;
using namespace vw::Barrier;

class ResourceTrackerTest : public ::testing::Test {
  protected:
    ResourceTracker tracker;

    const std::vector<vk::BufferMemoryBarrier2> &
    getPendingBufferBarriers() const {
        return tracker.pending_buffer_barriers();
    }

    const std::vector<vk::ImageMemoryBarrier2> &
    getPendingImageBarriers() const {
        return tracker.pending_image_barriers();
    }

    const std::vector<vk::MemoryBarrier2> &getPendingMemoryBarriers() const {
        return tracker.pending_memory_barriers();
    }

    struct BufferStateInfo {
        BufferInterval interval;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
    };

    std::vector<BufferStateInfo> getBufferStates(vk::Buffer buffer) {
        std::vector<BufferStateInfo> result;
        auto &states = tracker.buffer_states();
        auto it = states.find(buffer);
        if (it == states.end()) {
            return result;
        }

        for (const auto &stateSet : it->second) {
            for (const auto &interval : stateSet.intervals.intervals()) {
                result.push_back(
                    {interval, stateSet.state.stage, stateSet.state.access});
            }
        }
        std::sort(result.begin(), result.end(),
                  [](const BufferStateInfo &a, const BufferStateInfo &b) {
                      return a.interval.offset < b.interval.offset;
                  });
        return result;
    }

    struct ImageStateInfo {
        ImageInterval interval;
        vk::ImageLayout layout;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
    };

    std::vector<ImageStateInfo> getImageStates(vk::Image image) {
        std::vector<ImageStateInfo> result;
        auto &states = tracker.image_states();
        auto it = states.find(image);
        if (it == states.end()) {
            return result;
        }

        for (const auto &stateSet : it->second) {
            for (const auto &interval : stateSet.intervals.intervals()) {
                result.push_back({interval, stateSet.state.layout,
                                  stateSet.state.stage, stateSet.state.access});
            }
        }
        return result;
    }

    void clearPendingBarriers() { tracker.flush(vk::CommandBuffer{}); }
};

// =================================================================================================
// Buffer Tests
// =================================================================================================

TEST_F(ResourceTrackerTest, Buffer_UntrackedRequest_GeneratesFullBarrier) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    tracker.request(BufferState{.buffer = buffer,
                                .offset = 0,
                                .size = 1024,
                                .stage = vk::PipelineStageFlagBits2::eTransfer,
                                .access = vk::AccessFlagBits2::eTransferWrite});

    auto barriers = getPendingBufferBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].srcStageMask,
              vk::PipelineStageFlagBits2::eAllCommands);
    EXPECT_EQ(barriers[0].srcAccessMask, vk::AccessFlagBits2::eMemoryWrite |
                                             vk::AccessFlagBits2::eMemoryRead);
    EXPECT_EQ(barriers[0].dstStageMask, vk::PipelineStageFlagBits2::eTransfer);
    EXPECT_EQ(barriers[0].dstAccessMask, vk::AccessFlagBits2::eTransferWrite);

    auto states = getBufferStates(buffer);
    ASSERT_EQ(states.size(), 1);
    EXPECT_EQ(states[0].interval.offset, 0);
    EXPECT_EQ(states[0].interval.size, 1024);
    EXPECT_EQ(states[0].stage, vk::PipelineStageFlagBits2::eTransfer);
    EXPECT_EQ(states[0].access, vk::AccessFlagBits2::eTransferWrite);
}

TEST_F(ResourceTrackerTest, Buffer_RAR_NoBarrier) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    tracker.track(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 1024,
                    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    tracker.request(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 1024,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    EXPECT_TRUE(getPendingBufferBarriers().empty());

    auto states = getBufferStates(buffer);
    ASSERT_EQ(states.size(), 1);
    EXPECT_EQ(states[0].stage, vk::PipelineStageFlagBits2::eVertexShader);
    EXPECT_EQ(states[0].access, vk::AccessFlagBits2::eShaderRead);
}

TEST_F(ResourceTrackerTest, Buffer_RAW_GeneratesBarrier) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    tracker.track(BufferState{.buffer = buffer,
                              .offset = 0,
                              .size = 1024,
                              .stage = vk::PipelineStageFlagBits2::eTransfer,
                              .access = vk::AccessFlagBits2::eTransferWrite});

    tracker.request(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 1024,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    auto barriers = getPendingBufferBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].srcStageMask, vk::PipelineStageFlagBits2::eTransfer);
    EXPECT_EQ(barriers[0].srcAccessMask, vk::AccessFlagBits2::eTransferWrite);
    EXPECT_EQ(barriers[0].dstStageMask,
              vk::PipelineStageFlagBits2::eVertexShader);
    EXPECT_EQ(barriers[0].dstAccessMask, vk::AccessFlagBits2::eShaderRead);
    EXPECT_EQ(barriers[0].offset, 0);
    EXPECT_EQ(barriers[0].size, 1024);

    auto states = getBufferStates(buffer);
    ASSERT_EQ(states.size(), 1);
    EXPECT_EQ(states[0].access, vk::AccessFlagBits2::eShaderRead);
}

TEST_F(ResourceTrackerTest, Buffer_WAR_GeneratesBarrier) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    tracker.track(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 1024,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

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

    tracker.track(BufferState{.buffer = buffer,
                              .offset = 0,
                              .size = 1024,
                              .stage = vk::PipelineStageFlagBits2::eTransfer,
                              .access = vk::AccessFlagBits2::eTransferWrite});

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

    tracker.track(BufferState{.buffer = buffer,
                              .offset = 0,
                              .size = 1024,
                              .stage = vk::PipelineStageFlagBits2::eTransfer,
                              .access = vk::AccessFlagBits2::eTransferWrite});

    tracker.request(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 512,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    auto barriers = getPendingBufferBarriers();
    ASSERT_EQ(barriers.size(), 1);
    EXPECT_EQ(barriers[0].offset, 0);
    EXPECT_EQ(barriers[0].size, 512);

    auto states = getBufferStates(buffer);
    ASSERT_EQ(states.size(), 2);

    EXPECT_EQ(states[0].interval.offset, 0);
    EXPECT_EQ(states[0].interval.size, 512);
    EXPECT_EQ(states[0].access, vk::AccessFlagBits2::eShaderRead);

    EXPECT_EQ(states[1].interval.offset, 512);
    EXPECT_EQ(states[1].interval.size, 512);
    EXPECT_EQ(states[1].access, vk::AccessFlagBits2::eTransferWrite);
}

TEST_F(ResourceTrackerTest, Buffer_MergeStates) {
    vk::Buffer buffer = vk::Buffer(reinterpret_cast<VkBuffer>(0x100));

    tracker.track(
        BufferState{.buffer = buffer,
                    .offset = 0,
                    .size = 512,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

    tracker.track(
        BufferState{.buffer = buffer,
                    .offset = 512,
                    .size = 512,
                    .stage = vk::PipelineStageFlagBits2::eVertexShader,
                    .access = vk::AccessFlagBits2::eShaderRead});

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

    tracker.track(ImageState{.image = image,
                             .subresourceRange = range,
                             .layout = vk::ImageLayout::eUndefined,
                             .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
                             .access = vk::AccessFlagBits2::eNone});

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

    tracker.track(
        ImageState{.image = image,
                   .subresourceRange = range,
                   .layout = vk::ImageLayout::eColorAttachmentOptimal,
                   .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                   .access = vk::AccessFlagBits2::eColorAttachmentWrite});

    tracker.request(
        ImageState{.image = image,
                   .subresourceRange = range,
                   .layout = vk::ImageLayout::eColorAttachmentOptimal,
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
    vk::ImageSubresourceRange fullRange{vk::ImageAspectFlagBits::eColor, 0, 3,
                                        0, 1};

    tracker.track(ImageState{.image = image,
                             .subresourceRange = fullRange,
                             .layout = vk::ImageLayout::eTransferDstOptimal,
                             .stage = vk::PipelineStageFlagBits2::eTransfer,
                             .access = vk::AccessFlagBits2::eTransferWrite});

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
    EXPECT_EQ(barriers[0].subresourceRange.baseMipLevel, 1);
    EXPECT_EQ(barriers[0].subresourceRange.levelCount, 1);
    EXPECT_EQ(barriers[0].newLayout, vk::ImageLayout::eShaderReadOnlyOptimal);

    auto states = getImageStates(image);

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

    vk::ImageSubresourceRange mip0Range{vk::ImageAspectFlagBits::eColor, 0, 1,
                                        0, 1};
    tracker.track(ImageState{.image = image,
                             .subresourceRange = mip0Range,
                             .layout = vk::ImageLayout::eTransferDstOptimal,
                             .stage = vk::PipelineStageFlagBits2::eTransfer,
                             .access = vk::AccessFlagBits2::eTransferWrite});

    vk::ImageSubresourceRange requestRange{vk::ImageAspectFlagBits::eColor, 0,
                                           2, 0, 1};
    tracker.request(
        ImageState{.image = image,
                   .subresourceRange = requestRange,
                   .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
                   .stage = vk::PipelineStageFlagBits2::eFragmentShader,
                   .access = vk::AccessFlagBits2::eShaderRead});

    auto barriers = getPendingImageBarriers();
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

    vk::ImageSubresourceRange requestRange{vk::ImageAspectFlagBits::eColor, 0,
                                           1, 2, 1};
    tracker.request(ImageState{.image = image,
                               .subresourceRange = requestRange,
                               .layout = vk::ImageLayout::eTransferDstOptimal,
                               .stage = vk::PipelineStageFlagBits2::eTransfer,
                               .access = vk::AccessFlagBits2::eTransferWrite});

    auto barriers = getPendingImageBarriers();
    ASSERT_EQ(barriers.size(), 1);

    EXPECT_EQ(barriers[0].subresourceRange.baseArrayLayer, 2);
    EXPECT_EQ(barriers[0].subresourceRange.layerCount, 1);
    EXPECT_EQ(barriers[0].oldLayout, vk::ImageLayout::eUndefined);
    EXPECT_EQ(barriers[0].newLayout, vk::ImageLayout::eTransferDstOptimal);
    EXPECT_EQ(barriers[0].srcStageMask, vk::PipelineStageFlagBits2::eNone);
    EXPECT_EQ(barriers[0].srcAccessMask, vk::AccessFlagBits2::eNone);
    EXPECT_EQ(barriers[0].dstStageMask, vk::PipelineStageFlagBits2::eTransfer);
    EXPECT_EQ(barriers[0].dstAccessMask, vk::AccessFlagBits2::eTransferWrite);
}

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

    tracker.track(AccelerationStructureState{
        .handle = as,
        .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
        .access = vk::AccessFlagBits2::eAccelerationStructureWriteKHR});

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

    tracker.track(AccelerationStructureState{
        .handle = as,
        .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
        .access = vk::AccessFlagBits2::eAccelerationStructureWriteKHR});

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
