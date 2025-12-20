#include "VulkanWrapper/Command/CommandBuffer.h"

#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/PipelineLayout.h"

namespace vw {

CommandBufferRecorder::CommandBufferRecorder(vk::CommandBuffer commandBuffer)
    : m_commandBuffer{commandBuffer} {
    std::ignore = m_commandBuffer.begin(vk::CommandBufferBeginInfo());
}

CommandBufferRecorder::~CommandBufferRecorder() {
    std::ignore = m_commandBuffer.end();
}

void CommandBufferRecorder::traceRaysKHR(
    const vk::StridedDeviceAddressRegionKHR &raygenShaderBindingTable,
    const vk::StridedDeviceAddressRegionKHR &missShaderBindingTable,
    const vk::StridedDeviceAddressRegionKHR &hitShaderBindingTable,
    const vk::StridedDeviceAddressRegionKHR &callableShaderBindingTable,
    uint32_t width, uint32_t height, uint32_t depth) {
    m_commandBuffer.traceRaysKHR(
        raygenShaderBindingTable, missShaderBindingTable, hitShaderBindingTable,
        callableShaderBindingTable, width, height, depth);
}

} // namespace vw
