#pragma once

#include "VulkanWrapper/Image/Framebuffer.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"

namespace vw {

class RenderPassCommandBufferRecorder {
    friend class CommandBufferRecorder;

  public:
    RenderPassCommandBufferRecorder(RenderPassCommandBufferRecorder &&) =
        delete;
    RenderPassCommandBufferRecorder(const RenderPassCommandBufferRecorder &) =
        delete;
    RenderPassCommandBufferRecorder &
    operator=(const RenderPassCommandBufferRecorder &) = delete;
    RenderPassCommandBufferRecorder &
    operator=(RenderPassCommandBufferRecorder &&) = delete;
    ~RenderPassCommandBufferRecorder();

  private:
    RenderPassCommandBufferRecorder(vk::CommandBuffer commandBuffer);

    vk::CommandBuffer m_commandBuffer;
};

class CommandBufferRecorder {
  public:
    CommandBufferRecorder(vk::CommandBuffer commandBuffer);
    CommandBufferRecorder(CommandBufferRecorder &&) = delete;
    CommandBufferRecorder(const CommandBufferRecorder &) = delete;
    CommandBufferRecorder &operator=(const CommandBufferRecorder &) = delete;
    CommandBufferRecorder &operator=(CommandBufferRecorder &&) = delete;
    ~CommandBufferRecorder();

    RenderPassCommandBufferRecorder
    begin_render_pass(const IRenderPass &render_pass,
                      const vw::Framebuffer &framebuffer);

    void buildAccelerationStructure(
        const vk::AccelerationStructureBuildGeometryInfoKHR &buildInfo,
        std::span<const vk::AccelerationStructureBuildRangeInfoKHR>
            buildRangeInfos);

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
