#pragma once

#include "VulkanWrapper/fwd.h"
#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <vector>
#include <variant>

namespace vw::Barrier {

struct ImageState {
    const Image &image;
    vk::ImageLayout layout;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

struct BufferState {
    const BufferBase &buffer;
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
    struct InternalImageState {
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
        vk::PipelineStageFlags2 stage = vk::PipelineStageFlagBits2::eNone;
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;
    };

    struct InternalResourceState {
        vk::PipelineStageFlags2 stage = vk::PipelineStageFlagBits2::eNone;
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;
    };

    struct HandleHash {
        std::size_t operator()(const vk::Image &handle) const {
            return std::hash<uint64_t>()((uint64_t)(VkImage)handle);
        }
        std::size_t operator()(const vk::Buffer &handle) const {
            return std::hash<uint64_t>()((uint64_t)(VkBuffer)handle);
        }
        std::size_t operator()(const vk::AccelerationStructureKHR &handle) const {
            return std::hash<uint64_t>()((uint64_t)(VkAccelerationStructureKHR)handle);
        }
    };

    std::unordered_map<vk::Image, InternalImageState, HandleHash> m_image_states;
    std::unordered_map<vk::Buffer, InternalResourceState, HandleHash> m_buffer_states;
    std::unordered_map<vk::AccelerationStructureKHR, InternalResourceState, HandleHash>
        m_as_states;

    std::vector<vk::ImageMemoryBarrier2> m_pending_image_barriers;
    std::vector<vk::BufferMemoryBarrier2> m_pending_buffer_barriers;
    std::vector<vk::MemoryBarrier2> m_pending_memory_barriers;

    void track_image(const Image &image, vk::ImageLayout layout,
                    vk::PipelineStageFlags2 stage, vk::AccessFlags2 access);
    void track_buffer(const BufferBase &buffer, vk::PipelineStageFlags2 stage,
                     vk::AccessFlags2 access);
    void track_acceleration_structure(vk::AccelerationStructureKHR handle,
                                     vk::PipelineStageFlags2 stage,
                                     vk::AccessFlags2 access);

    void request_image(const Image &image, vk::ImageLayout layout,
                      vk::PipelineStageFlags2 stage, vk::AccessFlags2 access);
    void request_buffer(const BufferBase &buffer, vk::PipelineStageFlags2 stage,
                       vk::AccessFlags2 access);
    void request_acceleration_structure(vk::AccelerationStructureKHR handle,
                                       vk::PipelineStageFlags2 stage,
                                       vk::AccessFlags2 access);
};

} // namespace vw::Barrier
