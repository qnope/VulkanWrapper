#pragma once

#include "VulkanWrapper/Memory/Buffer.h"

namespace vw {

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
