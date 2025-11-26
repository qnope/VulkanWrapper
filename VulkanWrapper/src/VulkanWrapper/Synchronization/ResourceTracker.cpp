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
    m_image_states[{image, subresourceRange}] = {initialLayout, stage, access};
}

void ResourceTracker::track_buffer(vk::Buffer buffer, vk::DeviceSize offset,
                                   vk::DeviceSize size,
                                   vk::PipelineStageFlags2 stage,
                                   vk::AccessFlags2 access) {
    m_buffer_states[{buffer, offset, size}] = {stage, access};
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
    auto &currentState =
        m_image_states.try_emplace({image, subresourceRange}).first->second;

    if (currentState.layout != layout || currentState.stage != stage ||
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
        barrier.subresourceRange = subresourceRange;

        m_pending_image_barriers.push_back(barrier);

        currentState.layout = layout;
        currentState.stage = stage;
        currentState.access = access;
    }
}

void ResourceTracker::request_buffer(vk::Buffer buffer, vk::DeviceSize offset,
                                     vk::DeviceSize size,
                                     vk::PipelineStageFlags2 stage,
                                     vk::AccessFlags2 access) {
    auto &currentState =
        m_buffer_states.try_emplace({buffer, offset, size}).first->second;

    // Check if current state has any write access
    const bool currentHasWrite =
        (currentState.access & (vk::AccessFlagBits2::eMemoryWrite |
                                vk::AccessFlagBits2::eShaderWrite |
                                vk::AccessFlagBits2::eTransferWrite |
                                vk::AccessFlagBits2::eHostWrite)) !=
        vk::AccessFlagBits2::eNone;

    // Check if new access has any write
    const bool newHasWrite = (access & (vk::AccessFlagBits2::eMemoryWrite |
                                        vk::AccessFlagBits2::eShaderWrite |
                                        vk::AccessFlagBits2::eTransferWrite |
                                        vk::AccessFlagBits2::eHostWrite)) !=
                             vk::AccessFlagBits2::eNone;

    // Need barrier if:
    // - Write-after-Write (WAW)
    // - Write-after-Read (WAR)
    // - Read-after-Write (RAW)
    // Don't need barrier for Read-after-Read (RAR)
    const bool needBarrier = currentHasWrite || newHasWrite;

    if (needBarrier) {
        vk::BufferMemoryBarrier2 barrier;
        barrier.srcStageMask = currentState.stage;
        barrier.srcAccessMask = currentState.access;
        barrier.dstStageMask = stage;
        barrier.dstAccessMask = access;
        barrier.buffer = buffer;
        barrier.offset = offset;
        barrier.size = size;

        m_pending_buffer_barriers.push_back(barrier);

        currentState.stage = stage;
        currentState.access = access;
    }
}

void ResourceTracker::request_acceleration_structure(
    vk::AccelerationStructureKHR handle, vk::PipelineStageFlags2 stage,
    vk::AccessFlags2 access) {
    auto &currentState = m_as_states.try_emplace(handle).first->second;

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
