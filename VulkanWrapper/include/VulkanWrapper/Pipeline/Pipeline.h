#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
using GraphicsPipelineCreationException =
    TaggedException<struct GraphicsPipelineCreationTag>;

class Pipeline : public ObjectWithUniqueHandle<vk::UniquePipeline> {
    friend class GraphicsPipelineBuilder;

  public:
  private:
    using ObjectWithUniqueHandle<vk::UniquePipeline>::ObjectWithUniqueHandle;
};

class GraphicsPipelineBuilder {
  public:
    GraphicsPipelineBuilder(const Device &device, const RenderPass &renderPass);

    GraphicsPipelineBuilder &&add_shader(vk::ShaderStageFlagBits flags,
                                         ShaderModule module) &&;

    GraphicsPipelineBuilder &&add_dynamic_state(vk::DynamicState state) &&;

    GraphicsPipelineBuilder &&with_fixed_viewport(int width, int height) &&;
    GraphicsPipelineBuilder &&with_fixed_scissor(int width, int height) &&;

    GraphicsPipelineBuilder &&add_color_attachment() &&;

    GraphicsPipelineBuilder &&
    with_pipeline_layout(const PipelineLayout &pipelineLayout) &&;

    Pipeline build() &&;

  private:
    std::vector<vk::PipelineShaderStageCreateInfo>
    createShaderStageInfos() const noexcept;

    vk::PipelineDynamicStateCreateInfo createDynamicStateInfo() const noexcept;

    vk::PipelineVertexInputStateCreateInfo
    createVertexInputStateInfo() const noexcept;

    vk::PipelineInputAssemblyStateCreateInfo
    createInputAssemblyStateInfo() const noexcept;

    vk::PipelineViewportStateCreateInfo
    createViewportStateInfo() const noexcept;

    vk::PipelineRasterizationStateCreateInfo
    createRasterizationStateInfo() const noexcept;

    vk::PipelineMultisampleStateCreateInfo
    createMultisampleStateInfo() const noexcept;

    vk::PipelineColorBlendStateCreateInfo
    createColorBlendStateInfo() const noexcept;

  private:
    const Device &m_device;
    const RenderPass &m_renderPass;
    std::map<vk::ShaderStageFlagBits, ShaderModule> m_shaderModules;
    std::vector<vk::DynamicState> m_dynamicStates;

    std::optional<vk::Viewport> m_viewport;
    std::optional<vk::Rect2D> m_scissor;
    std::vector<vk::PipelineColorBlendAttachmentState> m_colorAttachmentStates;

    const PipelineLayout *m_pipelineLayout;
};
} // namespace vw
