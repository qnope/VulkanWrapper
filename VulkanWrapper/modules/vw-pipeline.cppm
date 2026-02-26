module;
#include "VulkanWrapper/3rd_party.h"

export module vw:pipeline;
import "VulkanWrapper/vw_vulkan.h";
import :core;
import :utils;
import :vulkan;
import :synchronization;
import :image;
import :memory;
import :descriptors;

export namespace vw {


// ---- PipelineLayout ----

class PipelineLayout : public ObjectWithUniqueHandle<vk::UniquePipelineLayout> {
    friend class PipelineLayoutBuilder;

  public:
  private:
    using ObjectWithUniqueHandle<
        vk::UniquePipelineLayout>::ObjectWithUniqueHandle;
};

class PipelineLayoutBuilder {
  public:
    PipelineLayoutBuilder(std::shared_ptr<const Device> device);

    PipelineLayoutBuilder &with_descriptor_set_layout(
        std::shared_ptr<const DescriptorSetLayout> layout);

    PipelineLayoutBuilder &
    with_push_constant_range(vk::PushConstantRange range);

    PipelineLayout build();

  private:
    [[nodiscard]] vk::UniqueDescriptorSetLayout build_set_layout(
        const std::vector<vk::DescriptorSetLayoutBinding> &bindings) const;

    std::shared_ptr<const Device> m_device;
    std::vector<std::shared_ptr<const DescriptorSetLayout>>
        m_descriptorSetLayouts;
    std::vector<vk::PushConstantRange> m_pushConstantRanges;
};

// ---- ShaderModule ----

class ShaderModule : public ObjectWithUniqueHandle<vk::UniqueShaderModule> {
  public:
    static std::shared_ptr<const ShaderModule>
    create_from_spirv(std::shared_ptr<const Device> device,
                      std::span<const std::uint32_t> spirv);

    static std::shared_ptr<const ShaderModule>
    create_from_spirv_file(std::shared_ptr<const Device> device,
                           const std::filesystem::path &path);

  private:
    using ObjectWithUniqueHandle<
        vk::UniqueShaderModule>::ObjectWithUniqueHandle;
};

// ---- ColorBlendConfig ----

struct ColorBlendConfig {
    vk::BlendFactor srcColorBlendFactor = vk::BlendFactor::eOne;
    vk::BlendFactor dstColorBlendFactor = vk::BlendFactor::eZero;
    vk::BlendOp colorBlendOp = vk::BlendOp::eAdd;
    vk::BlendFactor srcAlphaBlendFactor = vk::BlendFactor::eOne;
    vk::BlendFactor dstAlphaBlendFactor = vk::BlendFactor::eZero;
    vk::BlendOp alphaBlendOp = vk::BlendOp::eAdd;
    bool useDynamicConstants = false;

    static ColorBlendConfig constant_blend() {
        return {.srcColorBlendFactor = vk::BlendFactor::eConstantColor,
                .dstColorBlendFactor = vk::BlendFactor::eOneMinusConstantColor,
                .colorBlendOp = vk::BlendOp::eAdd,
                .srcAlphaBlendFactor = vk::BlendFactor::eOne,
                .dstAlphaBlendFactor = vk::BlendFactor::eZero,
                .alphaBlendOp = vk::BlendOp::eAdd,
                .useDynamicConstants = true};
    }

    static ColorBlendConfig alpha() {
        return {.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
                .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
                .colorBlendOp = vk::BlendOp::eAdd,
                .srcAlphaBlendFactor = vk::BlendFactor::eOne,
                .dstAlphaBlendFactor = vk::BlendFactor::eZero,
                .alphaBlendOp = vk::BlendOp::eAdd,
                .useDynamicConstants = false};
    }

    static ColorBlendConfig additive() {
        return {.srcColorBlendFactor = vk::BlendFactor::eOne,
                .dstColorBlendFactor = vk::BlendFactor::eOne,
                .colorBlendOp = vk::BlendOp::eAdd,
                .srcAlphaBlendFactor = vk::BlendFactor::eOne,
                .dstAlphaBlendFactor = vk::BlendFactor::eOne,
                .alphaBlendOp = vk::BlendOp::eAdd,
                .useDynamicConstants = false};
    }
};

// ---- Pipeline ----

class Pipeline : public ObjectWithUniqueHandle<vk::UniquePipeline> {
    friend class GraphicsPipelineBuilder;

  public:
    Pipeline(vk::UniquePipeline pipeline,
             PipelineLayout pipeline_layout) noexcept;

    [[nodiscard]] const PipelineLayout &layout() const noexcept;

  private:
    PipelineLayout m_layout;
};

// ---- GraphicsPipelineBuilder ----

class GraphicsPipelineBuilder {
  public:
    GraphicsPipelineBuilder(std::shared_ptr<const Device> device,
                            PipelineLayout pipelineLayout);

    GraphicsPipelineBuilder &
    add_shader(vk::ShaderStageFlagBits flags,
               std::shared_ptr<const ShaderModule> module);

    GraphicsPipelineBuilder &add_dynamic_state(vk::DynamicState state);

    GraphicsPipelineBuilder &with_fixed_viewport(int width, int height);
    GraphicsPipelineBuilder &with_fixed_scissor(int width, int height);
    GraphicsPipelineBuilder &with_dynamic_viewport_scissor();

    GraphicsPipelineBuilder &
    add_color_attachment(vk::Format format,
                         std::optional<ColorBlendConfig> blend = std::nullopt);

    GraphicsPipelineBuilder &set_depth_format(vk::Format format);
    GraphicsPipelineBuilder &set_stencil_format(vk::Format format);

    template <Vertex V> GraphicsPipelineBuilder &add_vertex_binding() {
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
        return *this;
    }

    GraphicsPipelineBuilder &with_depth_test(bool write,
                                             vk::CompareOp compare_operator);

    GraphicsPipelineBuilder &with_topology(vk::PrimitiveTopology topology);

    GraphicsPipelineBuilder &with_cull_mode(vk::CullModeFlags cull_mode);

    std::shared_ptr<const Pipeline> build();

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

    [[nodiscard]] vk::PipelineRasterizationStateCreateInfo
    createRasterizationStateInfo() noexcept;

    [[nodiscard]] static vk::PipelineMultisampleStateCreateInfo
    createMultisampleStateInfo() noexcept;

    [[nodiscard]] vk::PipelineColorBlendStateCreateInfo
    createColorBlendStateInfo() const noexcept;

    [[nodiscard]] vk::PipelineDepthStencilStateCreateInfo
    createDepthStencilStateInfo() const noexcept;

    std::shared_ptr<const Device> m_device;
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
    vk::CullModeFlags m_cullMode = vk::CullModeFlagBits::eBack;
};

// ---- ComputePipelineBuilder ----

class ComputePipelineBuilder {
  public:
    ComputePipelineBuilder(std::shared_ptr<const Device> device,
                           PipelineLayout pipelineLayout);

    ComputePipelineBuilder &
    set_shader(std::shared_ptr<const ShaderModule> module);

    std::shared_ptr<const Pipeline> build();

  private:
    std::shared_ptr<const Device> m_device;
    PipelineLayout m_pipelineLayout;
    std::shared_ptr<const ShaderModule> m_shaderModule;
};

// MeshRenderer is declared in :model partition (depends on Model::Mesh and
// Material types)

} // namespace vw
