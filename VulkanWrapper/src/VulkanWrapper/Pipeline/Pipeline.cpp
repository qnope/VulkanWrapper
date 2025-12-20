#include "VulkanWrapper/Pipeline/Pipeline.h"

#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

Pipeline::Pipeline(vk::UniquePipeline pipeline,
                   PipelineLayout pipeline_layout) noexcept
    : ObjectWithUniqueHandle<vk::UniquePipeline>{std::move(pipeline)}
    , m_layout{std::move(pipeline_layout)} {}

const PipelineLayout &Pipeline::layout() const noexcept { return m_layout; }

GraphicsPipelineBuilder::GraphicsPipelineBuilder(
    std::shared_ptr<const Device> device, PipelineLayout pipelineLayout)
    : m_device{std::move(device)}
    , m_pipelineLayout{std::move(pipelineLayout)} {}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::add_shader(
    vk::ShaderStageFlagBits flags,
    std::shared_ptr<const ShaderModule> module) {
    m_shaderModules.emplace(flags, std::move(module));
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::add_dynamic_state(vk::DynamicState state) {
    m_dynamicStates.push_back(state);
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::with_fixed_viewport(int width, int height) {
    m_viewport = vk::Viewport(0.0F, 0.0F, static_cast<float>(width),
                              static_cast<float>(height), 0.0F, 1.0F);
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::with_fixed_scissor(int width, int height) {
    m_scissor = vk::Rect2D(vk::Offset2D(), vk::Extent2D(width, height));
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::with_dynamic_viewport_scissor() {
    m_dynamicStates.push_back(vk::DynamicState::eViewport);
    m_dynamicStates.push_back(vk::DynamicState::eScissor);
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::add_color_attachment(
    vk::Format format, std::optional<ColorBlendConfig> blend) {

    auto colorBlendAttachment =
        vk::PipelineColorBlendAttachmentState().setColorWriteMask(
            vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eR);

    if (blend) {
        colorBlendAttachment.setBlendEnable(vk::True)
            .setSrcColorBlendFactor(blend->srcColorBlendFactor)
            .setDstColorBlendFactor(blend->dstColorBlendFactor)
            .setColorBlendOp(blend->colorBlendOp)
            .setSrcAlphaBlendFactor(blend->srcAlphaBlendFactor)
            .setDstAlphaBlendFactor(blend->dstAlphaBlendFactor)
            .setAlphaBlendOp(blend->alphaBlendOp);

        if (blend->useDynamicConstants) {
            m_dynamicStates.push_back(vk::DynamicState::eBlendConstants);
        }
    } else {
        colorBlendAttachment.setBlendEnable(vk::False);
    }

    m_colorAttachmentStates.push_back(colorBlendAttachment);
    m_colorAttachmentFormats.push_back(format);
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::set_depth_format(vk::Format format) {
    m_depthFormat = format;
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::set_stencil_format(vk::Format format) {
    m_stencilFormat = format;
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::with_depth_test(bool write,
                                         vk::CompareOp compare_operator) {
    m_depthTestEnabled = static_cast<vk::Bool32>(true);
    m_depthWriteEnabled = static_cast<vk::Bool32>(write);
    m_depthCompareOp = compare_operator;
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::with_topology(vk::PrimitiveTopology topology) {
    m_topology = topology;
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::with_cull_mode(vk::CullModeFlags cull_mode) {
    m_cullMode = cull_mode;
    return *this;
}

std::shared_ptr<const Pipeline> GraphicsPipelineBuilder::build() {
    const auto shaderStageInfos = createShaderStageInfos();
    const auto dynamicStateInfo = createDynamicStateInfo();
    const auto viewportStateInfo = createViewportStateInfo();
    const auto colorBlendStateInfo = createColorBlendStateInfo();
    const auto multisampleStateInfo = createMultisampleStateInfo();
    const auto vertexInputStateInfo = createVertexInputStateInfo();
    const auto depthStencilStateInfo = createDepthStencilStateInfo();
    const auto inputAssemblyStateInfo = createInputAssemblyStateInfo();
    const auto rasterizationStateInfo = createRasterizationStateInfo();

    auto renderingInfo = vk::PipelineRenderingCreateInfo()
                             .setColorAttachmentFormats(m_colorAttachmentFormats)
                             .setDepthAttachmentFormat(m_depthFormat)
                             .setStencilAttachmentFormat(m_stencilFormat);

    const auto info = vk::GraphicsPipelineCreateInfo()
                          .setPNext(&renderingInfo)
                          .setStages(shaderStageInfos)
                          .setPDynamicState(&dynamicStateInfo)
                          .setPViewportState(&viewportStateInfo)
                          .setPColorBlendState(&colorBlendStateInfo)
                          .setPVertexInputState(&vertexInputStateInfo)
                          .setPMultisampleState(&multisampleStateInfo)
                          .setPDepthStencilState(&depthStencilStateInfo)
                          .setPInputAssemblyState(&inputAssemblyStateInfo)
                          .setPRasterizationState(&rasterizationStateInfo)
                          .setLayout(m_pipelineLayout.handle());

    auto pipeline = check_vk(
        m_device->handle().createGraphicsPipelineUnique(vk::PipelineCache(), info),
        "Failed to create graphics pipeline");

    return std::make_shared<Pipeline>(std::move(pipeline),
                                      std::move(m_pipelineLayout));
}

std::vector<vk::PipelineShaderStageCreateInfo>
GraphicsPipelineBuilder::createShaderStageInfos() const noexcept {
    std::vector<vk::PipelineShaderStageCreateInfo> infos;

    for (const auto &[stage, module] : m_shaderModules) {
        auto info = vk::PipelineShaderStageCreateInfo()
                        .setPName("main")
                        .setStage(stage)
                        .setModule(module->handle());
        infos.push_back(info);
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
    return vk::PipelineVertexInputStateCreateInfo()
        .setVertexAttributeDescriptions(m_input_attribute_descriptions)
        .setVertexBindingDescriptions(m_input_binding_descriptions);
}

vk::PipelineInputAssemblyStateCreateInfo
GraphicsPipelineBuilder::createInputAssemblyStateInfo() const noexcept {
    return vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(m_topology)
        .setPrimitiveRestartEnable(0U);
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
GraphicsPipelineBuilder::createRasterizationStateInfo() noexcept {
    return vk::PipelineRasterizationStateCreateInfo()
        .setDepthClampEnable(0U)
        .setRasterizerDiscardEnable(0U)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0F)
        .setCullMode(m_cullMode)
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .setDepthBiasEnable(0U);
}

vk::PipelineMultisampleStateCreateInfo
GraphicsPipelineBuilder::createMultisampleStateInfo() noexcept {
    return vk::PipelineMultisampleStateCreateInfo()
        .setSampleShadingEnable(0U)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1);
}

vk::PipelineColorBlendStateCreateInfo
GraphicsPipelineBuilder::createColorBlendStateInfo() const noexcept {
    return vk::PipelineColorBlendStateCreateInfo()
        .setAttachments(m_colorAttachmentStates)
        .setLogicOpEnable(0U);
}

vk::PipelineDepthStencilStateCreateInfo
GraphicsPipelineBuilder::createDepthStencilStateInfo() const noexcept {
    return vk::PipelineDepthStencilStateCreateInfo()
        .setDepthTestEnable(m_depthTestEnabled)
        .setDepthWriteEnable(m_depthWriteEnabled)
        .setDepthCompareOp(m_depthCompareOp);
}

} // namespace vw
