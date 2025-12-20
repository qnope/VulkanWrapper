#include "VulkanWrapper/Synchronization/ResourceTracker.h"

namespace vw::Barrier {

void ResourceTracker::track(const ResourceState &state) {
    std::visit(
        [this](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ImageState>) {
                track_image(arg.image, arg.subresourceRange, arg.layout,
                            arg.stage, arg.access);
            } else if constexpr (std::is_same_v<T, BufferState>) {
                track_buffer(arg.buffer, arg.offset, arg.size, arg.stage,
                             arg.access);
            } else if constexpr (std::is_same_v<T,
                                                AccelerationStructureState>) {
                track_acceleration_structure(arg.handle, arg.stage, arg.access);
            }
        },
        state);
}

void ResourceTracker::request(const ResourceState &state) {
    std::visit(
        [this](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ImageState>) {
                request_image(arg.image, arg.subresourceRange, arg.layout,
                              arg.stage, arg.access);
            } else if constexpr (std::is_same_v<T, BufferState>) {
                request_buffer(arg.buffer, arg.offset, arg.size, arg.stage,
                               arg.access);
            } else if constexpr (std::is_same_v<T,
                                                AccelerationStructureState>) {
                request_acceleration_structure(arg.handle, arg.stage,
                                               arg.access);
            }
        },
        state);
}

void ResourceTracker::track_image(vk::Image image,
                                  vk::ImageSubresourceRange subresourceRange,
                                  vk::ImageLayout initialLayout,
                                  vk::PipelineStageFlags2 stage,
                                  vk::AccessFlags2 access) {
    ImageInterval interval(subresourceRange);
    InternalImageState state{initialLayout, stage, access};

    auto &stateSets = m_image_states[image];

    // Find existing state group or create new one
    for (auto &stateSet : stateSets) {
        if (stateSet.state.layout == state.layout &&
            stateSet.state.stage == state.stage &&
            stateSet.state.access == state.access) {
            // Add to existing state group
            stateSet.intervals.add(interval);
            return;
        }
    }

    // Create new state group
    ImageIntervalSetState newStateSet;
    newStateSet.state = state;
    newStateSet.intervals.add(interval);
    stateSets.push_back(std::move(newStateSet));
}

void ResourceTracker::track_buffer(vk::Buffer buffer, vk::DeviceSize offset,
                                   vk::DeviceSize size,
                                   vk::PipelineStageFlags2 stage,
                                   vk::AccessFlags2 access) {
    BufferInterval interval(offset, size);
    InternalBufferState state{stage, access};

    auto &stateSets = m_buffer_states[buffer];

    // Find existing state group or create new one
    for (auto &stateSet : stateSets) {
        if (stateSet.state.stage == state.stage &&
            stateSet.state.access == state.access) {
            // Add to existing state group
            stateSet.intervals.add(interval);
            return;
        }
    }

    // Create new state group
    BufferIntervalSetState newStateSet;
    newStateSet.state = state;
    newStateSet.intervals.add(interval);
    stateSets.push_back(std::move(newStateSet));
}

void ResourceTracker::track_acceleration_structure(
    vk::AccelerationStructureKHR handle, vk::PipelineStageFlags2 stage,
    vk::AccessFlags2 access) {
    m_as_states[handle] = {stage, access};
}

void ResourceTracker::request_image(vk::Image image,
                                    vk::ImageSubresourceRange subresourceRange,
                                    vk::ImageLayout layout,
                                    vk::PipelineStageFlags2 stage,
                                    vk::AccessFlags2 access) {
    ImageInterval requestedInterval(subresourceRange);
    auto &stateSets = m_image_states[image];

    // Track parts of the requested interval that are not covered by existing
    // states
    std::vector<ImageInterval> remainingIntervals;
    remainingIntervals.push_back(requestedInterval);

    // Check all state groups for overlaps
    for (auto &stateSet : stateSets) {
        auto overlapping =
            stateSet.intervals.findOverlapping(requestedInterval);

        if (!overlapping.empty()) {
            auto &currentState = stateSet.state;

            // Generate barriers for each overlapping interval
            for (const auto &overlap : overlapping) {
                // Intersect with requested interval to only barrier what's
                // needed
                auto intersection = overlap.intersect(requestedInterval);
                if (!intersection)
                    continue;

                // Subtract this intersection from remaining intervals
                std::vector<ImageInterval> nextRemaining;
                for (const auto &rem : remainingIntervals) {
                    auto diff = rem.difference(*intersection);
                    nextRemaining.insert(nextRemaining.end(), diff.begin(),
                                         diff.end());
                }
                remainingIntervals = std::move(nextRemaining);

                if (currentState.layout != layout ||
                    currentState.stage != stage ||
                    currentState.access != access ||
                    currentState.layout == vk::ImageLayout::eUndefined) {

                    vk::ImageMemoryBarrier2 barrier;
                    barrier.srcStageMask = currentState.stage;
                    barrier.srcAccessMask = currentState.access;
                    barrier.dstStageMask = stage;
                    barrier.dstAccessMask = access;
                    barrier.oldLayout = currentState.layout;
                    barrier.newLayout = layout;
                    barrier.image = image;
                    barrier.subresourceRange = intersection->range;

                    m_pending_image_barriers.push_back(barrier);
                }
            }

            // Remove overlapping intervals from old state
            stateSet.intervals.remove(requestedInterval);
        }
    }

    // Generate barriers for untracked (remaining) intervals (Undefined -> New
    // Layout)
    for (const auto &interval : remainingIntervals) {
        // Only generate barrier if we are transitioning to a specific layout
        // (Undefined -> Undefined doesn't need a barrier, but Undefined ->
        // Something does)
        if (layout != vk::ImageLayout::eUndefined) {
            vk::ImageMemoryBarrier2 barrier;
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
            barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
            barrier.dstStageMask = stage;
            barrier.dstAccessMask = access;
            barrier.oldLayout = vk::ImageLayout::eUndefined;
            barrier.newLayout = layout;
            barrier.image = image;
            barrier.subresourceRange = interval.range;

            m_pending_image_barriers.push_back(barrier);
        }
    }

    // Add to new state group
    track_image(image, subresourceRange, layout, stage, access);
}

void ResourceTracker::request_buffer(vk::Buffer buffer, vk::DeviceSize offset,
                                     vk::DeviceSize size,
                                     vk::PipelineStageFlags2 stage,
                                     vk::AccessFlags2 access) {
    BufferInterval requestedInterval(offset, size);
    auto &stateSets = m_buffer_states[buffer];

    // Check if new access has any write
    const bool newHasWrite = (access & (vk::AccessFlagBits2::eMemoryWrite |
                                        vk::AccessFlagBits2::eShaderWrite |
                                        vk::AccessFlagBits2::eTransferWrite |
                                        vk::AccessFlagBits2::eHostWrite)) !=
                             vk::AccessFlagBits2::eNone;

    // Check all state groups for overlaps
    bool foundOverlap = false;
    for (auto &stateSet : stateSets) {
        auto overlapping =
            stateSet.intervals.findOverlapping(requestedInterval);

        if (!overlapping.empty()) {
            foundOverlap = true;
            auto &currentState = stateSet.state;

            // Check if current state has any write access
            const bool currentHasWrite =
                (currentState.access & (vk::AccessFlagBits2::eMemoryWrite |
                                        vk::AccessFlagBits2::eShaderWrite |
                                        vk::AccessFlagBits2::eTransferWrite |
                                        vk::AccessFlagBits2::eHostWrite)) !=
                vk::AccessFlagBits2::eNone;

            // Need barrier if: WAW, WAR, or RAW (not RAR)
            const bool needBarrier = currentHasWrite || newHasWrite;

            if (needBarrier) {
                // Generate barriers for each overlapping interval
                for (const auto &overlap : overlapping) {
                    // Intersect with requested interval to only barrier what's
                    // needed
                    auto intersection = overlap.intersect(requestedInterval);
                    if (!intersection)
                        continue;

                    vk::BufferMemoryBarrier2 barrier;
                    barrier.srcStageMask = currentState.stage;
                    barrier.srcAccessMask = currentState.access;
                    barrier.dstStageMask = stage;
                    barrier.dstAccessMask = access;
                    barrier.buffer = buffer;
                    barrier.offset = intersection->offset;
                    barrier.size = intersection->size;

                    m_pending_buffer_barriers.push_back(barrier);
                }
            }

            // Remove overlapping intervals from old state
            stateSet.intervals.remove(requestedInterval);
        }
    }

    // If no overlap found (untracked), assume undefined state and generate full
    // barrier if needed Actually, for buffers, untracked usually means we don't
    // know the state. User requested: "When a buffer is requested, if there is
    // no entry for this buffer, we should assume full memory barrier."
    if (!foundOverlap) {
        // Full memory barrier for the requested range
        // We assume the previous state could be anything (Undefined is not
        // quite right for buffers, but effectively we need to wait for
        // everything) However, we can't really put "Undefined" stage/access for
        // buffers in a useful way that matches "Full Memory Barrier" logic
        // without using AllCommands/AllRead/AllWrite.

        // Let's use AllCommands -> Target Stage, MemoryRead|Write -> Target
        // Access Or simply srcStage=AllCommands, srcAccess=MemoryWrite?

        vk::BufferMemoryBarrier2 barrier;
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
        barrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite |
                                vk::AccessFlagBits2::eMemoryRead;
        // Wait, if we don't know previous state, we must assume worst case: it
        // was written to by any stage.

        barrier.dstStageMask = stage;
        barrier.dstAccessMask = access;
        barrier.buffer = buffer;
        barrier.offset = offset;
        barrier.size = size;

        m_pending_buffer_barriers.push_back(barrier);
    }

    // Add to new state group
    track_buffer(buffer, offset, size, stage, access);
}

void ResourceTracker::request_acceleration_structure(
    vk::AccelerationStructureKHR handle, vk::PipelineStageFlags2 stage,
    vk::AccessFlags2 access) {

    auto it = m_as_states.find(handle);
    if (it == m_as_states.end()) {
        // Untracked AS: Assume full memory barrier
        vk::MemoryBarrier2 barrier;
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
        barrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite |
                                vk::AccessFlagBits2::eMemoryRead;
        barrier.dstStageMask = stage;
        barrier.dstAccessMask = access;

        m_pending_memory_barriers.push_back(barrier);

        m_as_states[handle] = {stage, access};
        return;
    }

    auto &currentState = it->second;

    // Check if current state has any write access
    const bool currentHasWrite =
        (currentState.access &
         (vk::AccessFlagBits2::eAccelerationStructureWriteKHR |
          vk::AccessFlagBits2::eTransferWrite)) != vk::AccessFlagBits2::eNone;

    // Check if new access has any write
    const bool newHasWrite =
        (access & (vk::AccessFlagBits2::eAccelerationStructureWriteKHR |
                   vk::AccessFlagBits2::eTransferWrite)) !=
        vk::AccessFlagBits2::eNone;

    // Need barrier if:
    // - Write-after-Write (WAW)
    // - Write-after-Read (WAR)
    // - Read-after-Write (RAW)
    // Don't need barrier for Read-after-Read (RAR)
    const bool needBarrier = currentHasWrite || newHasWrite;

    if (needBarrier) {
        vk::MemoryBarrier2 barrier;
        barrier.srcStageMask = currentState.stage;
        barrier.srcAccessMask = currentState.access;
        barrier.dstStageMask = stage;
        barrier.dstAccessMask = access;

        m_pending_memory_barriers.push_back(barrier);

        currentState.stage = stage;
        currentState.access = access;
    }
}

void ResourceTracker::flush(vk::CommandBuffer commandBuffer) {
    if (m_pending_image_barriers.empty() && m_pending_buffer_barriers.empty() &&
        m_pending_memory_barriers.empty()) {
        return;
    }

    vk::DependencyInfo dependencyInfo;
    dependencyInfo.setImageMemoryBarriers(m_pending_image_barriers);
    dependencyInfo.setBufferMemoryBarriers(m_pending_buffer_barriers);
    dependencyInfo.setMemoryBarriers(m_pending_memory_barriers);

    commandBuffer.pipelineBarrier2(dependencyInfo);

    m_pending_image_barriers.clear();
    m_pending_buffer_barriers.clear();
    m_pending_memory_barriers.clear();
}

} // namespace vw::Barrier
