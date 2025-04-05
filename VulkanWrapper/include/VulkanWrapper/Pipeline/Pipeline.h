#pragma once

#include "VulkanWrapper/Descriptors/Vertex.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
using GraphicsPipelineCreationException =
    TaggedException<struct GraphicsPipelineCreationTag>;

class Pipeline : public ObjectWithUniqueHandle<vk::UniquePipeline> {
    friend class GraphicsPipelineBuilder;

  public:
    Pipeline(vk::UniquePipeline pipeline,
             PipelineLayout pipeline_layout) noexcept;

    [[nodiscard]] const PipelineLayout &layout() const noexcept;

  private:
    PipelineLayout m_layout;
};

class GraphicsPipelineBuilder {
  public:
    GraphicsPipelineBuilder(const Device &device, const RenderPass &renderPass,
                            uint32_t subpass_index,
                            PipelineLayout pipelineLayout);

    GraphicsPipelineBuilder &&
    add_shader(vk::ShaderStageFlagBits flags,
               std::shared_ptr<const ShaderModule> module) &&;

    GraphicsPipelineBuilder &&add_dynamic_state(vk::DynamicState state) &&;

    GraphicsPipelineBuilder &&with_fixed_viewport(int width, int height) &&;
    GraphicsPipelineBuilder &&with_fixed_scissor(int width, int height) &&;

    GraphicsPipelineBuilder &&add_color_attachment() &&;

    template <Vertex V> GraphicsPipelineBuilder &&add_vertex_binding() && {
        const auto binding = m_input_binding_descriptions.size();
        const auto location = [this] {
            if (m_input_attribute_descriptions.empty()) {
                return 0U;
            }
            return static_cast<unsigned>(
                m_input_attribute_descriptions.back().location + 1);
        }();

        m_input_binding_descriptions.push_back(V::binding_description(binding));

        for (auto attribute : V::attribute_descriptions(binding, location)) {
            m_input_attribute_descriptions.push_back(attribute);
        }
        return std::move(*this);
    }

    GraphicsPipelineBuilder &&
    with_depth_test(bool write, vk::CompareOp compare_operator) &&;

    Pipeline build() &&;

  private:
    [[nodiscard]] std::vector<vk::PipelineShaderStageCreateInfo>
    createShaderStageInfos() const noexcept;

    [[nodiscard]] vk::PipelineDynamicStateCreateInfo
    createDynamicStateInfo() const noexcept;

    [[nodiscard]] vk::PipelineVertexInputStateCreateInfo
    createVertexInputStateInfo() const noexcept;

    [[nodiscard]] static vk::PipelineInputAssemblyStateCreateInfo
    createInputAssemblyStateInfo() noexcept;

    [[nodiscard]] vk::PipelineViewportStateCreateInfo
    createViewportStateInfo() const noexcept;

    [[nodiscard]] static vk::PipelineRasterizationStateCreateInfo
    createRasterizationStateInfo() noexcept;

    [[nodiscard]] static vk::PipelineMultisampleStateCreateInfo
    createMultisampleStateInfo() noexcept;

    [[nodiscard]] vk::PipelineColorBlendStateCreateInfo
    createColorBlendStateInfo() const noexcept;

    [[nodiscard]] vk::PipelineDepthStencilStateCreateInfo
    createDepthStencilStateInfo() const noexcept;

    const Device *m_device;
    const RenderPass *m_renderPass;
    uint32_t m_subpass_index;
    PipelineLayout m_pipelineLayout;

    std::map<vk::ShaderStageFlagBits, std::shared_ptr<const ShaderModule>>
        m_shaderModules;
    std::vector<vk::DynamicState> m_dynamicStates;

    std::optional<vk::Viewport> m_viewport;
    std::optional<vk::Rect2D> m_scissor;
    std::vector<vk::PipelineColorBlendAttachmentState> m_colorAttachmentStates;

    std::vector<vk::VertexInputBindingDescription> m_input_binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription>
        m_input_attribute_descriptions;

    vk::Bool32 m_depthTestEnabled = vk::Bool32{false};
    vk::Bool32 m_depthWriteEnabled = vk::Bool32{false};
    vk::CompareOp m_depthCompareOp = vk::CompareOp::eLess;
};
} // namespace vw
