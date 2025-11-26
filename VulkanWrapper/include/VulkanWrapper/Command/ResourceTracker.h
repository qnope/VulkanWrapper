#pragma once

#include "VulkanWrapper/fwd.h"
#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <vector>

namespace vw {

class BufferBase;
namespace rt::as {
class BottomLevelAccelerationStructure;
class TopLevelAccelerationStructure;
}

class ResourceTracker {
  public:
    ResourceTracker() = default;

    void track(const Image &image, vk::ImageLayout initialLayout,
               vk::PipelineStageFlags2 stage, vk::AccessFlags2 access);
    void track(const BufferBase &buffer, vk::PipelineStageFlags2 stage,
               vk::AccessFlags2 access);
    void track(const rt::as::BottomLevelAccelerationStructure &blas,
               vk::PipelineStageFlags2 stage, vk::AccessFlags2 access);
    void track(const rt::as::TopLevelAccelerationStructure &tlas,
               vk::PipelineStageFlags2 stage, vk::AccessFlags2 access);

    void request(const Image &image, vk::ImageLayout layout,
                 vk::PipelineStageFlags2 stage, vk::AccessFlags2 access);
    void request(const BufferBase &buffer, vk::PipelineStageFlags2 stage,
                 vk::AccessFlags2 access);
    void request(const rt::as::BottomLevelAccelerationStructure &blas,
                 vk::PipelineStageFlags2 stage, vk::AccessFlags2 access);
    void request(const rt::as::TopLevelAccelerationStructure &tlas,
                 vk::PipelineStageFlags2 stage, vk::AccessFlags2 access);

    void flush(vk::CommandBuffer commandBuffer);

  private:

    struct ImageState {
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
        vk::PipelineStageFlags2 stage = vk::PipelineStageFlagBits2::eNone;
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;
    };

    struct ResourceState {
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

    std::unordered_map<vk::Image, ImageState, HandleHash> m_image_states;
    std::unordered_map<vk::Buffer, ResourceState, HandleHash> m_buffer_states;
    std::unordered_map<vk::AccelerationStructureKHR, ResourceState, HandleHash>
        m_as_states;

    std::vector<vk::ImageMemoryBarrier2> m_pending_image_barriers;
    std::vector<vk::BufferMemoryBarrier2> m_pending_buffer_barriers;
    std::vector<vk::MemoryBarrier2> m_pending_memory_barriers;

    void request_acceleration_structure(vk::AccelerationStructureKHR handle,
                                        vk::PipelineStageFlags2 stage,
                                        vk::AccessFlags2 access);
};

} // namespace vw
