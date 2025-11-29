#pragma once
#include "VulkanWrapper/3rd_party.h"

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
    GraphicsPipelineBuilder(const Device &device,
                            PipelineLayout pipelineLayout);

    GraphicsPipelineBuilder &&
    add_shader(vk::ShaderStageFlagBits flags,
               std::shared_ptr<const ShaderModule> module) &&;

    GraphicsPipelineBuilder &&add_dynamic_state(vk::DynamicState state) &&;

    GraphicsPipelineBuilder &&with_fixed_viewport(int width, int height) &&;
    GraphicsPipelineBuilder &&with_fixed_scissor(int width, int height) &&;
    GraphicsPipelineBuilder &&with_dynamic_viewport_scissor() &&;

    GraphicsPipelineBuilder &&add_color_attachment(vk::Format format) &&;
    GraphicsPipelineBuilder &&set_depth_format(vk::Format format) &&;
    GraphicsPipelineBuilder &&set_stencil_format(vk::Format format) &&;

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

    GraphicsPipelineBuilder &&with_topology(vk::PrimitiveTopology topology) &&;

    std::shared_ptr<const Pipeline> build() &&;

  private:
    [[nodiscard]] std::vector<vk::PipelineShaderStageCreateInfo>
    createShaderStageInfos() const noexcept;

    [[nodiscard]] vk::PipelineDynamicStateCreateInfo
    createDynamicStateInfo() const noexcept;

    [[nodiscard]] vk::PipelineVertexInputStateCreateInfo
    createVertexInputStateInfo() const noexcept;

    [[nodiscard]] vk::PipelineInputAssemblyStateCreateInfo
    createInputAssemblyStateInfo() const noexcept;

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
    PipelineLayout m_pipelineLayout;

    std::map<vk::ShaderStageFlagBits, std::shared_ptr<const ShaderModule>>
        m_shaderModules;
    std::vector<vk::DynamicState> m_dynamicStates;

    std::optional<vk::Viewport> m_viewport;
    std::optional<vk::Rect2D> m_scissor;
    std::vector<vk::PipelineColorBlendAttachmentState> m_colorAttachmentStates;
    std::vector<vk::Format> m_colorAttachmentFormats;
    vk::Format m_depthFormat = vk::Format::eUndefined;
    vk::Format m_stencilFormat = vk::Format::eUndefined;

    std::vector<vk::VertexInputBindingDescription> m_input_binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription>
        m_input_attribute_descriptions;

    vk::Bool32 m_depthTestEnabled = vk::Bool32{false};
    vk::Bool32 m_depthWriteEnabled = vk::Bool32{false};
    vk::CompareOp m_depthCompareOp = vk::CompareOp::eLess;
    vk::PrimitiveTopology m_topology = vk::PrimitiveTopology::eTriangleList;
};
} // namespace vw
