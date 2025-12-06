#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/IntervalSet.h"
#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <vector>
#include <variant>

namespace vw::Barrier {

struct ImageState {
    vk::Image image;
    vk::ImageSubresourceRange subresourceRange;
    vk::ImageLayout layout;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

struct BufferState {
    vk::Buffer buffer;
    vk::DeviceSize offset;
    vk::DeviceSize size;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

struct AccelerationStructureState {
    vk::AccelerationStructureKHR handle;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

using ResourceState = std::variant<ImageState, BufferState, AccelerationStructureState>;

class ResourceTracker {
  public:
    ResourceTracker() = default;

    void track(const ResourceState &state);
    void request(const ResourceState &state);
    void flush(vk::CommandBuffer commandBuffer);

  private:
    friend class ResourceTrackerTest;

    struct InternalImageState {
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
        vk::PipelineStageFlags2 stage = vk::PipelineStageFlagBits2::eNone;
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;
    };

    struct InternalBufferState {
        vk::PipelineStageFlags2 stage = vk::PipelineStageFlagBits2::eNone;
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;
    };

    struct InternalAccelerationStructureState {
        vk::PipelineStageFlags2 stage = vk::PipelineStageFlagBits2::eNone;
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;
    };

    struct HandleHash {
        std::size_t operator()(const vk::AccelerationStructureKHR &handle) const {
            return std::hash<uint64_t>()((uint64_t)(VkAccelerationStructureKHR)handle);
        }
    };

    struct BufferHandleHash {
        std::size_t operator()(const vk::Buffer &buffer) const {
            return std::hash<uint64_t>()((uint64_t)(VkBuffer)buffer);
        }
    };
    
    struct ImageHandleHash {
        std::size_t operator()(const vk::Image &image) const {
            return std::hash<uint64_t>()((uint64_t)(VkImage)image);
        }
    };

    // Group intervals by state: vector<pair<IntervalSet, State>>
    struct BufferIntervalSetState {
        BufferIntervalSet intervals;
        InternalBufferState state;
    };

    struct ImageIntervalSetState {
        ImageIntervalSet intervals;
        InternalImageState state;
    };

    std::unordered_map<vk::Buffer, std::vector<BufferIntervalSetState>, BufferHandleHash> m_buffer_states;
    std::unordered_map<vk::Image, std::vector<ImageIntervalSetState>, ImageHandleHash> m_image_states;
    std::unordered_map<vk::AccelerationStructureKHR, InternalAccelerationStructureState, HandleHash> m_as_states;

    std::vector<vk::ImageMemoryBarrier2> m_pending_image_barriers;
    std::vector<vk::BufferMemoryBarrier2> m_pending_buffer_barriers;
    std::vector<vk::MemoryBarrier2> m_pending_memory_barriers;

    void track_image(vk::Image image, vk::ImageSubresourceRange subresourceRange,
                     vk::ImageLayout layout, vk::PipelineStageFlags2 stage,
                     vk::AccessFlags2 access);
    void track_buffer(vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size,
                      vk::PipelineStageFlags2 stage, vk::AccessFlags2 access);
    void track_acceleration_structure(vk::AccelerationStructureKHR handle,
                                     vk::PipelineStageFlags2 stage,
                                     vk::AccessFlags2 access);

    void request_image(vk::Image image, vk::ImageSubresourceRange subresourceRange,
                       vk::ImageLayout layout, vk::PipelineStageFlags2 stage,
                       vk::AccessFlags2 access);
    void request_buffer(vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size,
                        vk::PipelineStageFlags2 stage, vk::AccessFlags2 access);
    void request_acceleration_structure(vk::AccelerationStructureKHR handle,
                                       vk::PipelineStageFlags2 stage,
                                       vk::AccessFlags2 access);
};

} // namespace vw::Barrier
