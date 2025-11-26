#pragma once

#include "VulkanWrapper/fwd.h"
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
    struct ImageKey {
        vk::Image handle;
        vk::ImageSubresourceRange subresourceRange;

        bool operator==(const ImageKey &) const = default;
    };

    struct BufferKey {
        vk::Buffer handle;
        vk::DeviceSize offset;
        vk::DeviceSize size;

        bool operator==(const BufferKey &) const = default;
    };

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

    struct ImageKeyHash {
        std::size_t operator()(const ImageKey &key) const {
            return std::hash<uint64_t>()((uint64_t)(VkImage)key.handle);
        }
    };

    struct BufferKeyHash {
        std::size_t operator()(const BufferKey &key) const {
            return std::hash<uint64_t>()((uint64_t)(VkBuffer)key.handle);
        }
    };

    struct HandleHash {
        std::size_t operator()(const vk::AccelerationStructureKHR &handle) const {
            return std::hash<uint64_t>()((uint64_t)(VkAccelerationStructureKHR)handle);
        }
    };

    std::unordered_map<ImageKey, InternalImageState, ImageKeyHash> m_image_states;
    std::unordered_map<BufferKey, InternalBufferState, BufferKeyHash> m_buffer_states;
    std::unordered_map<vk::AccelerationStructureKHR, InternalAccelerationStructureState, HandleHash>
        m_as_states;

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
