#include "VulkanWrapper/Command/CommandBuffer.h"

#include "VulkanWrapper/Image/Framebuffer.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"

namespace vw {

PipelineBoundCommandBufferRecorder::PipelineBoundCommandBufferRecorder(
    vk::CommandBuffer commandBuffer)
    : m_commandBuffer{commandBuffer} {}

PipelineBoundCommandBufferRecorder &
PipelineBoundCommandBufferRecorder::bind_descriptor_set(
    const PipelineLayout &layout, int first_set,
    std::span<const vk::DescriptorSet> sets,
    std::span<const uint32_t> dynamic_offsets) {
    m_commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                       layout.handle(), first_set, sets,
                                       dynamic_offsets);
    return *this;
}

void PipelineBoundCommandBufferRecorder::draw(uint32_t numberVertex,
                                              uint32_t numberInstance,
                                              uint32_t firstVertex,
                                              uint32_t firstInstance) {
    m_commandBuffer.draw(numberVertex, numberInstance, firstVertex,
                         firstInstance);
}

RenderPassCommandBufferRecorder::~RenderPassCommandBufferRecorder() {
    m_commandBuffer.endRenderPass();
}

PipelineBoundCommandBufferRecorder
RenderPassCommandBufferRecorder::bind_graphics_pipeline(
    const Pipeline &pipeline) {
    m_commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                 pipeline.handle());
    return PipelineBoundCommandBufferRecorder{m_commandBuffer};
}

RenderPassCommandBufferRecorder::RenderPassCommandBufferRecorder(
    vk::CommandBuffer commandBuffer)
    : m_commandBuffer{commandBuffer} {}

CommandBufferRecorder::CommandBufferRecorder(vk::CommandBuffer commandBuffer)
    : m_commandBuffer{commandBuffer} {
    std::ignore = m_commandBuffer.begin(vk::CommandBufferBeginInfo());
}

CommandBufferRecorder::~CommandBufferRecorder() {
    std::ignore = m_commandBuffer.end();
}

RenderPassCommandBufferRecorder
CommandBufferRecorder::begin_render_pass(const RenderPass &render_pass,
                                         const Framebuffer &framebuffer) {
    vk::ClearValue color;

    const auto renderPassBeginInfo =
        vk::RenderPassBeginInfo()
            .setRenderPass(render_pass.handle())
            .setFramebuffer(framebuffer.handle())
            .setRenderArea(
                vk::Rect2D(vk::Offset2D(), vk::Extent2D{framebuffer.width(),
                                                        framebuffer.height()}))
            .setClearValues(color);

    const auto subpassInfo =
        vk::SubpassBeginInfo().setContents(vk::SubpassContents::eInline);

    m_commandBuffer.beginRenderPass2(renderPassBeginInfo, subpassInfo);
    return RenderPassCommandBufferRecorder{m_commandBuffer};
}

} // namespace vw
