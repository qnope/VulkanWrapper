#pragma once

#include "VulkanWrapper/Image/Framebuffer.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"

namespace vw {

class PipelineBoundCommandBufferRecorder {
    friend class RenderPassCommandBufferRecorder;

  public:
    template <typename T, bool HV, VkBufferUsageFlags Usage>
    PipelineBoundCommandBufferRecorder &
    bind_vertex_buffer(int binding, const Buffer<T, HV, Usage> &buffer) {
        static_assert((Usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) ==
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      "Buffer must be a Vertex Buffer");
        auto handle = buffer.handle();
        vk::DeviceSize offset = 0;
        m_commandBuffer.bindVertexBuffers(binding, handle, offset);
        return *this;
    }

    template <VkBufferUsageFlags Usage>
    PipelineBoundCommandBufferRecorder &
    bind_index_buffer(const Buffer<unsigned, false, Usage> &buffer) {
        static_assert((Usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) ==
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      "Buffer must be an Index Buffer");
        auto handle = buffer.handle();
        m_commandBuffer.bindIndexBuffer(handle, 0, vk::IndexType::eUint32);
        return *this;
    }

    PipelineBoundCommandBufferRecorder &
    bind_descriptor_set(const PipelineLayout &layout, int first_set,
                        std::span<const vk::DescriptorSet> sets,
                        std::span<const uint32_t> dynamic_offsets);

    void draw(uint32_t numberVertex, uint32_t numberInstance,
              uint32_t firstVertex, uint32_t firstInstance) const;

    void indexed_draw(uint32_t index_count, uint32_t instance_count,
                      uint32_t first_index, int32_t vertex_offset,
                      uint32_t first_instance) const {
        m_commandBuffer.drawIndexed(index_count, instance_count, first_index,
                                    vertex_offset, first_instance);
    }

  public:
    PipelineBoundCommandBufferRecorder(vk::CommandBuffer commandBuffer);

    vk::CommandBuffer m_commandBuffer;
};

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
    PipelineBoundCommandBufferRecorder
    bind_graphics_pipeline(const Pipeline &pipeline);

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
    begin_render_pass(const RenderPass &render_pass,
                      const vw::Framebuffer &framebuffer);

  private:
    vk::CommandBuffer m_commandBuffer;
};
} // namespace vw
