#include "VulkanWrapper/Pipeline/Pipeline.h"

#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

Pipeline::Pipeline(vk::UniquePipeline pipeline,
                   PipelineLayout pipeline_layout) noexcept
    : ObjectWithUniqueHandle<vk::UniquePipeline>{std::move(pipeline)}
    , m_layout{std::move(pipeline_layout)} {}

const PipelineLayout &Pipeline::layout() const noexcept { return m_layout; }

GraphicsPipelineBuilder::GraphicsPipelineBuilder(const Device &device,
                                                 const RenderPass &renderPass,
                                                 PipelineLayout pipelineLayout)
    : m_device{&device}
    , m_renderPass{&renderPass}
    , m_pipelineLayout{std::move(pipelineLayout)} {}

GraphicsPipelineBuilder &&
GraphicsPipelineBuilder::add_shader(vk::ShaderStageFlagBits flags,
                                    std::shared_ptr<ShaderModule> module) && {
    m_shaderModules.emplace(flags, std::move(module));
    return std::move(*this);
}

GraphicsPipelineBuilder &&
GraphicsPipelineBuilder::add_dynamic_state(vk::DynamicState state) && {
    m_dynamicStates.push_back(state);
    return std::move(*this);
}

GraphicsPipelineBuilder &&
GraphicsPipelineBuilder::with_fixed_viewport(int width, int height) && {
    m_viewport = vk::Viewport(0.0F, 0.0F, static_cast<float>(width),
                              static_cast<float>(height), 0.0F, 1.0F);
    return std::move(*this);
}

GraphicsPipelineBuilder &&
GraphicsPipelineBuilder::with_fixed_scissor(int width, int height) && {
    m_scissor = vk::Rect2D(vk::Offset2D(), vk::Extent2D(width, height));
    return std::move(*this);
}

GraphicsPipelineBuilder &&GraphicsPipelineBuilder::add_color_attachment() && {
    const auto colorBlendAttachment =
        vk::PipelineColorBlendAttachmentState()
            .setBlendEnable(0U)
            .setColorWriteMask(vk::ColorComponentFlagBits::eA |
                               vk::ColorComponentFlagBits::eB |
                               vk::ColorComponentFlagBits::eG |
                               vk::ColorComponentFlagBits::eR);
    m_colorAttachmentStates.push_back(colorBlendAttachment);
    return std::move(*this);
}

GraphicsPipelineBuilder &&
GraphicsPipelineBuilder::with_depth_test(bool write,
                                         vk::CompareOp compare_operator) && {
    m_depthTestEnabled = static_cast<vk::Bool32>(true);
    m_depthWriteEnabled = static_cast<vk::Bool32>(write);
    m_depthCompareOp = compare_operator;
    return std::move(*this);
}

Pipeline GraphicsPipelineBuilder::build() && {
    const auto shaderStageInfos = createShaderStageInfos();
    const auto dynamicStateInfo = createDynamicStateInfo();
    const auto viewportStateInfo = createViewportStateInfo();
    const auto colorBlendStateInfo = createColorBlendStateInfo();
    const auto multisampleStateInfo = createMultisampleStateInfo();
    const auto vertexInputStateInfo = createVertexInputStateInfo();
    const auto depthStencilStateInfo = createDepthStencilStateInfo();
    const auto inputAssemblyStateInfo = createInputAssemblyStateInfo();
    const auto rasterizationStateInfo = createRasterizationStateInfo();

    const auto info = vk::GraphicsPipelineCreateInfo()
                          .setStages(shaderStageInfos)
                          .setRenderPass(m_renderPass->handle())
                          .setPDynamicState(&dynamicStateInfo)
                          .setPViewportState(&viewportStateInfo)
                          .setPColorBlendState(&colorBlendStateInfo)
                          .setPVertexInputState(&vertexInputStateInfo)
                          .setPMultisampleState(&multisampleStateInfo)
                          .setPDepthStencilState(&depthStencilStateInfo)
                          .setPInputAssemblyState(&inputAssemblyStateInfo)
                          .setPRasterizationState(&rasterizationStateInfo)
                          .setLayout(m_pipelineLayout.handle());

    auto [result, pipeline] = m_device->handle().createGraphicsPipelineUnique(
        vk::PipelineCache(), info);

    if (result != vk::Result::eSuccess) {
        throw GraphicsPipelineCreationException{
            std::source_location::current()};
    }

    return Pipeline{std::move(pipeline), std::move(m_pipelineLayout)};
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
GraphicsPipelineBuilder::createInputAssemblyStateInfo() noexcept {
    return vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList)
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
        .setCullMode(vk::CullModeFlagBits::eBack)
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
