#include "VulkanWrapper/Command/CommandBuffer.h"

#include "VulkanWrapper/Image/Framebuffer.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"

namespace vw {

RenderPassCommandBufferRecorder::~RenderPassCommandBufferRecorder() {
    m_commandBuffer.endRenderPass();
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
    const auto &clear_values = render_pass.clear_values();

    const auto renderPassBeginInfo =
        vk::RenderPassBeginInfo()
            .setRenderPass(render_pass.handle())
            .setFramebuffer(framebuffer.handle())
            .setRenderArea(vk::Rect2D(vk::Offset2D(), framebuffer.extent2D()))
            .setClearValues(clear_values);

    const auto subpassInfo =
        vk::SubpassBeginInfo().setContents(vk::SubpassContents::eInline);

    m_commandBuffer.beginRenderPass2(renderPassBeginInfo, subpassInfo);
    return RenderPassCommandBufferRecorder{m_commandBuffer};
}

void CommandBufferRecorder::buildAccelerationStructure(
    const vk::AccelerationStructureBuildGeometryInfoKHR &buildInfo,
    std::span<const vk::AccelerationStructureBuildRangeInfoKHR>
        buildRangeInfos) {
    assert(buildInfo.geometryCount == buildRangeInfos.size());
    const auto pBuildRangeInfos = buildRangeInfos.data();
    m_commandBuffer.buildAccelerationStructuresKHR(1, &buildInfo,
                                                   &pBuildRangeInfos);
}

} // namespace vw
