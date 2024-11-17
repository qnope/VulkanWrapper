#include "VulkanWrapper/Pipeline/Pipeline.h"

#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
GraphicsPipelineBuilder
GraphicsPipelineBuilder::addShaderModule(vk::ShaderStageFlagBits flags,
                                         ShaderModule module) && {
    m_shaderModules.emplace(flags, std::move(module));
    return std::move(*this);
}

Pipeline GraphicsPipelineBuilder::build(Device &device) && {
    const auto shaderStageInfos = createShaderStageInfos();

    auto info = vk::GraphicsPipelineCreateInfo().setStages(shaderStageInfos);

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

} // namespace vw
