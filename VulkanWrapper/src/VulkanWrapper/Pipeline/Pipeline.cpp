#include "VulkanWrapper/Pipeline/Pipeline.h"

#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
GraphicsPipelineBuilder
GraphicsPipelineBuilder::addShaderModule(vk::ShaderStageFlagBits flags,
                                         ShaderModule module) && {
    m_shaderModules.emplace(flags, std::move(module));
    return std::move(*this);
}

GraphicsPipelineBuilder
GraphicsPipelineBuilder::addDynamicState(vk::DynamicState state) && {
    m_dynamicStates.push_back(state);
    return std::move(*this);
}

GraphicsPipelineBuilder
GraphicsPipelineBuilder::withFixedViewport(int width, int height) && {
    m_viewport = vk::Viewport(0.0f, 0.0f, width, height, 0.0f, 1.0f);
    return std::move(*this);
}

GraphicsPipelineBuilder
GraphicsPipelineBuilder::withFixedScissor(int width, int height) && {
    m_scissor = vk::Rect2D(vk::Offset2D(), vk::Extent2D(width, height));
    return std::move(*this);
}

GraphicsPipelineBuilder GraphicsPipelineBuilder::addColorAttachment() && {
    const auto colorBlendAttachment =
        vk::PipelineColorBlendAttachmentState().setBlendEnable(false);
    m_colorAttachmentStates.push_back(colorBlendAttachment);
    return std::move(*this);
}

GraphicsPipelineBuilder GraphicsPipelineBuilder::withPipelineLayout(
    const PipelineLayout &pipelineLayout) && {
    m_pipelineLayout = &pipelineLayout;
    return std::move(*this);
}

Pipeline GraphicsPipelineBuilder::build(Device &device) && {
    const auto shaderStageInfos = createShaderStageInfos();
    const auto dynamicStateInfo = createDynamicStateInfo();
    const auto viewportStateInfo = createViewportStateInfo();
    const auto colorBlendStateInfo = createColorBlendStateInfo();
    const auto multisampleStateInfo = createMultisampleStateInfo();
    const auto vertexInputStateInfo = createVertexInputStateInfo();
    const auto inputAssemblyStateInfo = createInputAssemblyStateInfo();
    const auto rasterizationStateInfo = createRasterizationStateInfo();

    const auto info =
        vk::GraphicsPipelineCreateInfo()
            .setStages(shaderStageInfos)
            .setPDynamicState(&dynamicStateInfo)
            .setPViewportState(&viewportStateInfo)
            .setPColorBlendState(&colorBlendStateInfo)
            .setPVertexInputState(&vertexInputStateInfo)
            .setPMultisampleState(&multisampleStateInfo)
            .setPInputAssemblyState(&inputAssemblyStateInfo)
            .setPRasterizationState(&rasterizationStateInfo)
            .setLayout(m_pipelineLayout ? m_pipelineLayout->handle() : nullptr);

    auto [result, pipeline] =
        device.handle().createGraphicsPipelineUnique(vk::PipelineCache(), info);

    if (result != vk::Result::eSuccess)
        throw GraphicsPipelineCreationException{
            std::source_location::current()};

    return Pipeline{std::move(pipeline)};
}

std::vector<vk::PipelineShaderStageCreateInfo>
GraphicsPipelineBuilder::createShaderStageInfos() const noexcept {
    std::vector<vk::PipelineShaderStageCreateInfo> infos;

    for (const auto &[stage, module] : m_shaderModules) {
        auto info = vk::PipelineShaderStageCreateInfo()
                        .setPName("main")
                        .setStage(stage)
                        .setModule(module.handle());
        infos.push_back(std::move(info));
    }

    return infos;
}

vk::PipelineDynamicStateCreateInfo
GraphicsPipelineBuilder::createDynamicStateInfo() const noexcept {
    return vk::PipelineDynamicStateCreateInfo().setDynamicStates(
        m_dynamicStates);
}

vk::PipelineVertexInputStateCreateInfo
GraphicsPipelineBuilder::createVertexInputStateInfo() const noexcept {
    return vk::PipelineVertexInputStateCreateInfo();
}

vk::PipelineInputAssemblyStateCreateInfo
GraphicsPipelineBuilder::createInputAssemblyStateInfo() const noexcept {
    return vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(false);
}

vk::PipelineViewportStateCreateInfo
GraphicsPipelineBuilder::createViewportStateInfo() const noexcept {
    return vk::PipelineViewportStateCreateInfo()
        .setViewportCount(1)
        .setPViewports(m_viewport ? &m_viewport.value() : nullptr)
        .setScissorCount(1)
        .setPScissors(m_scissor ? &m_scissor.value() : nullptr);
}

vk::PipelineRasterizationStateCreateInfo
GraphicsPipelineBuilder::createRasterizationStateInfo() const noexcept {
    return vk::PipelineRasterizationStateCreateInfo()
        .setDepthClampEnable(false)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0f)
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setDepthBiasEnable(false);
}

vk::PipelineMultisampleStateCreateInfo
GraphicsPipelineBuilder::createMultisampleStateInfo() const noexcept {
    return vk::PipelineMultisampleStateCreateInfo()
        .setSampleShadingEnable(false)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1);
}

vk::PipelineColorBlendStateCreateInfo
GraphicsPipelineBuilder::createColorBlendStateInfo() const noexcept {
    return vk::PipelineColorBlendStateCreateInfo()
        .setAttachments(m_colorAttachmentStates)
        .setLogicOpEnable(false);
}

} // namespace vw
