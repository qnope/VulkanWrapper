#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/Subpass.h"

struct ZPassTag {};
inline const auto z_pass_tag = vw::create_subpass_tag<ZPassTag>();

inline vw::Pipeline create_zpass_pipeline(
    const vw::Device &device, const vw::RenderPass &render_pass,
    std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout,
    vw::Width width, vw::Height height) {
    auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
        device, "../../../examples/Advanced/Shaders/bin/GBuffer/zpass.spv");

    auto pipeline_layout =
        vw::PipelineLayoutBuilder(device)
            .with_descriptor_set_layout(uniform_buffer_layout)
            .build();

    return vw::GraphicsPipelineBuilder(device, render_pass, 0,
                                       std::move(pipeline_layout))
        .add_vertex_binding<vw::Vertex3D>()
        .add_shader(vk::ShaderStageFlagBits::eVertex, std::move(vertex_shader))
        .with_fixed_scissor(int32_t(width), int32_t(height))
        .with_fixed_viewport(int32_t(width), int32_t(height))
        .with_depth_test(true, vk::CompareOp::eLess)
        .build();
}

class ZPass : public vw::Subpass {
  public:
    ZPass(const vw::Device &device, const vw::Model::MeshManager &mesh_manager,
          std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout,
          vw::Width width, vw::Height height, vk::DescriptorSet descriptor_set)
        : m_device{device}
        , m_mesh_manager{mesh_manager}
        , m_uniform_buffer_layout{uniform_buffer_layout}
        , m_width{width}
        , m_height{height}
        , m_descriptor_set{descriptor_set} {}

    void execute(vk::CommandBuffer cmd_buffer) const noexcept override {
        const auto &meshes = m_mesh_manager.meshes();
        std::span first_descriptor_sets = {&m_descriptor_set, 1};
        cmd_buffer.bindPipeline(pipeline_bind_point(), m_pipeline->handle());
        cmd_buffer.bindDescriptorSets(pipeline_bind_point(),
                                      m_pipeline->layout().handle(), 0,
                                      first_descriptor_sets, nullptr);
        for (const auto &mesh : meshes) {
            mesh.draw_zpass(cmd_buffer);
        }
    }

    const std::vector<vk::AttachmentReference2> &
    color_attachments() const noexcept override {
        return m_color_attachments;
    }

    const vk::AttachmentReference2 *
    depth_stencil_attachment() const noexcept override {
        return &m_depth_stencil_attachment;
    }

    vw::SubpassDependencyMask input_dependencies() const noexcept override {
        return {};
    }

    vw::SubpassDependencyMask output_dependencies() const noexcept override {
        vw::SubpassDependencyMask mask;
        mask.access = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        mask.stage = vk::PipelineStageFlagBits::eEarlyFragmentTests |
                     vk::PipelineStageFlagBits::eLateFragmentTests;
        return mask;
    }

  protected:
    void initialize(const vw::RenderPass &render_pass) override {
        m_pipeline = create_zpass_pipeline(
            m_device, render_pass, m_uniform_buffer_layout, m_width, m_height);
    }

  private:
    const vw::Device &m_device;
    const vw::Model::MeshManager &m_mesh_manager;
    std::shared_ptr<const vw::DescriptorSetLayout> m_uniform_buffer_layout;
    vw::Width m_width;
    vw::Height m_height;
    vk::DescriptorSet m_descriptor_set;
    std::optional<vw::Pipeline> m_pipeline;

    inline static const vk::AttachmentReference2 m_depth_stencil_attachment =
        vk::AttachmentReference2(
            1, vk::ImageLayout::eDepthStencilAttachmentOptimal,
            vk::ImageAspectFlagBits::eDepth);

    inline static const std::vector<vk::AttachmentReference2>
        m_color_attachments = {};
};
