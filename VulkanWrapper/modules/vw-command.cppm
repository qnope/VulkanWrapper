module;
#include "VulkanWrapper/3rd_party.h"
#include <cstddef>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

export module vw:command;
import :core;
import :utils;
import :vulkan;

export namespace vw {

// ---- CommandPool ----

class CommandPool : public ObjectWithUniqueHandle<vk::UniqueCommandPool> {
    friend class CommandPoolBuilder;

  public:
    std::vector<vk::CommandBuffer> allocate(std::size_t number);

  private:
    CommandPool(std::shared_ptr<const Device> device,
                vk::UniqueCommandPool commandPool);

    std::shared_ptr<const Device> m_device;
};

class CommandPoolBuilder {
  public:
    CommandPoolBuilder(std::shared_ptr<const Device> device);

    CommandPoolBuilder &with_reset_command_buffer();

    CommandPool build();

  private:
    std::shared_ptr<const Device> m_device;
    vk::CommandPoolCreateFlags m_flags{};
};

// ---- CommandBufferRecorder ----

class CommandBufferRecorder {
  public:
    CommandBufferRecorder(vk::CommandBuffer commandBuffer);
    CommandBufferRecorder(CommandBufferRecorder &&) = delete;
    CommandBufferRecorder(const CommandBufferRecorder &) = delete;
    CommandBufferRecorder &operator=(const CommandBufferRecorder &) = delete;
    CommandBufferRecorder &operator=(CommandBufferRecorder &&) = delete;
    ~CommandBufferRecorder();

    void traceRaysKHR(
        const vk::StridedDeviceAddressRegionKHR &raygenShaderBindingTable,
        const vk::StridedDeviceAddressRegionKHR &missShaderBindingTable,
        const vk::StridedDeviceAddressRegionKHR &hitShaderBindingTable,
        const vk::StridedDeviceAddressRegionKHR &callableShaderBindingTable,
        uint32_t width, uint32_t height, uint32_t depth);

  private:
    vk::CommandBuffer m_commandBuffer;
};

} // namespace vw
