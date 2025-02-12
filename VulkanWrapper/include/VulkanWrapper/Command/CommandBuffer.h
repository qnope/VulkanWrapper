#pragma once

#include "VulkanWrapper/Image/Framebuffer.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"

namespace vw {

class PipelineBoundCommandBufferRecorder {
    friend class RenderPassCommandBufferRecorder;

  public:
    void draw(uint32_t numberVertex, uint32_t numberInstance,
              uint32_t firstVertex, uint32_t firstInstance);

  private:
    PipelineBoundCommandBufferRecorder(vk::CommandBuffer commandBuffer);

  private:
    vk::CommandBuffer m_commandBuffer;
};

class RenderPassCommandBufferRecorder {
    friend class CommandBufferRecorder;

  public:
    ~RenderPassCommandBufferRecorder();
    PipelineBoundCommandBufferRecorder
    bind_graphics_pipeline(const Pipeline &pipeline);

  private:
    RenderPassCommandBufferRecorder(vk::CommandBuffer commandBuffer);

  private:
    vk::CommandBuffer m_commandBuffer;
};

class CommandBufferRecorder {
  public:
    CommandBufferRecorder(vk::CommandBuffer commandBuffer);
    ~CommandBufferRecorder();

    RenderPassCommandBufferRecorder
    begin_render_pass(const RenderPass &render_pass,
                      const vw::Framebuffer &framebuffer);

  private:
    vk::CommandBuffer m_commandBuffer;
};
} // namespace vw
