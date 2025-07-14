#pragma once

#include "RenderPassInformation.h"
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialManager.h"
#include "VulkanWrapper/Model/Material/TexturedMaterialManager.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/Pipeline/MeshRenderer.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/Subpass.h"

inline vw::Pipeline create_pipeline(
    const vw::Device &device, const vw::IRenderPass &render_pass,
    std::shared_ptr<const vw::ShaderModule> vertex,
    std::shared_ptr<const vw::ShaderModule> fragment,
    std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout,
    std::shared_ptr<const vw::DescriptorSetLayout> material_layout,
    vw::Width width, vw::Height height) {

    auto pipelineLayout = vw::PipelineLayoutBuilder(device)
                              .with_descriptor_set_layout(uniform_buffer_layout)
                              .with_descriptor_set_layout(material_layout)
                              .build();

    return vw::GraphicsPipelineBuilder(device, render_pass, 1,
                                       std::move(pipelineLayout))
        .add_vertex_binding<vw::FullVertex3D>()
        .add_shader(vk::ShaderStageFlagBits::eVertex, std::move(vertex))
        .add_shader(vk::ShaderStageFlagBits::eFragment, std::move(fragment))
        .with_fixed_scissor(int32_t(width), int32_t(height))
        .with_fixed_viewport(int32_t(width), int32_t(height))
        .with_depth_test(false, vk::CompareOp::eEqual)
        .add_color_attachment()
        .add_color_attachment()
        .add_color_attachment()
        .add_color_attachment()
        .add_color_attachment()
        .build();
}

inline vw::MeshRenderer create_renderer(
    const vw::Device &device, const vw::IRenderPass &render_pass,
    const vw::Model::MeshManager &mesh_manager,
    const std::shared_ptr<const vw::DescriptorSetLayout> &uniform_buffer_layout,
    vw::Width width, vw::Height height) {
    auto vertexShader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/GBuffer/gbuffer.spv");
    auto fragment_textured = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/GBuffer/gbuffer_textured.spv");
    auto fragment_colored = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/GBuffer/gbuffer_colored.spv");
    auto textured_pipeline =
        create_pipeline(device, render_pass, vertexShader, fragment_textured,
                        uniform_buffer_layout,
                        mesh_manager.material_manager_map().layout(
                            vw::Model::Material::textured_material_tag),
                        width, height);

    auto colored_pipeline =
        create_pipeline(device, render_pass, vertexShader, fragment_colored,
                        uniform_buffer_layout,
                        mesh_manager.material_manager_map().layout(
                            vw::Model::Material::colored_material_tag),
                        width, height);

    vw::MeshRenderer renderer;
    renderer.add_pipeline(vw::Model::Material::textured_material_tag,
                          std::move(textured_pipeline));
    renderer.add_pipeline(vw::Model::Material::colored_material_tag,
                          std::move(colored_pipeline));
    return renderer;
}

struct ColorPassTag {};
const auto color_pass_tag = vw::create_subpass_tag<ColorPassTag>();

class ColorSubpass : public vw::Subpass<GBufferInformation> {
  public:
    ColorSubpass(
        const vw::Device &device, const vw::Model::MeshManager &mesh_manager,
        std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout,
        vw::Width width, vw::Height height, vk::DescriptorSet descriptor_set)
        : m_device{device}
        , m_mesh_manager{mesh_manager}
        , m_uniform_buffer_layout{uniform_buffer_layout}
        , m_width{width}
        , m_height{height}
        , m_descriptor_set{descriptor_set} {}

    void
    execute(vk::CommandBuffer cmd_buffer,
            const GBufferInformation &information) const noexcept override {
        const auto &meshes = m_mesh_manager.meshes();
        std::span first_descriptor_sets = {&m_descriptor_set, 1};
        for (const auto &mesh : meshes) {
            m_mesh_renderer.draw_mesh(cmd_buffer, mesh, first_descriptor_sets);
        }
    }

    const std::vector<vk::AttachmentReference2> &
    color_attachments() const noexcept override {
        static const std::vector<vk::AttachmentReference2> color_attachments = {
            vk::AttachmentReference2(0,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageAspectFlagBits::eColor),
            vk::AttachmentReference2(1,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageAspectFlagBits::eColor),
            vk::AttachmentReference2(2,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageAspectFlagBits::eColor),
            vk::AttachmentReference2(3,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageAspectFlagBits::eColor),
            vk::AttachmentReference2(4,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageAspectFlagBits::eColor)};
        return color_attachments;
    }

    const vk::AttachmentReference2 *
    depth_stencil_attachment() const noexcept override {
        static const vk::AttachmentReference2 depth_stencil_attachment =
            vk::AttachmentReference2(
                6, vk::ImageLayout::eDepthStencilReadOnlyOptimal,
                vk::ImageAspectFlagBits::eDepth);
        return &depth_stencil_attachment;
    }

    vw::SubpassDependencyMask input_dependencies() const noexcept override {
        vw::SubpassDependencyMask mask;
        mask.access = vk::AccessFlagBits::eDepthStencilAttachmentRead;
        mask.stage = vk::PipelineStageFlagBits::eEarlyFragmentTests |
                     vk::PipelineStageFlagBits::eLateFragmentTests;
        return mask;
    }

    vw::SubpassDependencyMask output_dependencies() const noexcept override {
        vw::SubpassDependencyMask mask;
        mask.access = vk::AccessFlagBits::eColorAttachmentWrite;
        mask.stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        return mask;
    }

  protected:
    void initialize(const vw::IRenderPass &render_pass) override {
        m_mesh_renderer =
            create_renderer(m_device, render_pass, m_mesh_manager,
                            m_uniform_buffer_layout, m_width, m_height);
    }

  private:
    const vw::Device &m_device;
    const vw::Model::MeshManager &m_mesh_manager;
    std::shared_ptr<const vw::DescriptorSetLayout> m_uniform_buffer_layout;
    vw::Width m_width;
    vw::Height m_height;
    vw::MeshRenderer m_mesh_renderer;
    vk::DescriptorSet m_descriptor_set;
};
