#pragma once

#include "RenderPassInformation.h"
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/DescriptorSet.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialManager.h"
#include "VulkanWrapper/Model/Material/TexturedMaterialManager.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/Pipeline/MeshRenderer.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/Subpass.h"
#include <span>

inline vw::Pipeline create_pipeline(
    const vw::Device &device, std::span<const vk::Format> color_formats,
    vk::Format depth_format, std::shared_ptr<const vw::ShaderModule> vertex,
    std::shared_ptr<const vw::ShaderModule> fragment,
    std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout,
    std::shared_ptr<const vw::DescriptorSetLayout> material_layout) {

    auto pipelineLayout = vw::PipelineLayoutBuilder(device)
                              .with_descriptor_set_layout(uniform_buffer_layout)
                              .with_descriptor_set_layout(material_layout)
                              .build();

    auto builder =
        vw::GraphicsPipelineBuilder(device, std::move(pipelineLayout))
            .add_vertex_binding<vw::FullVertex3D>()
            .add_shader(vk::ShaderStageFlagBits::eVertex, std::move(vertex))
            .add_shader(vk::ShaderStageFlagBits::eFragment, std::move(fragment))
            .with_dynamic_viewport_scissor()
            .with_depth_test(false, vk::CompareOp::eEqual)
            .set_depth_format(depth_format);

    for (auto format : color_formats) {
        std::move(builder).add_color_attachment(format);
    }

    return std::move(builder).build();
}

inline vw::MeshRenderer create_renderer(
    const vw::Device &device, std::span<const vk::Format> color_formats,
    vk::Format depth_format, const vw::Model::MeshManager &mesh_manager,
    const std::shared_ptr<const vw::DescriptorSetLayout>
        &uniform_buffer_layout) {
    auto vertexShader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/GBuffer/gbuffer.spv");
    auto fragment_textured = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/GBuffer/gbuffer_textured.spv");
    auto fragment_colored = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/GBuffer/gbuffer_colored.spv");
    auto textured_pipeline =
        create_pipeline(device, color_formats, depth_format, vertexShader,
                        fragment_textured, uniform_buffer_layout,
                        mesh_manager.material_manager_map().layout(
                            vw::Model::Material::textured_material_tag));

    auto colored_pipeline =
        create_pipeline(device, color_formats, depth_format, vertexShader,
                        fragment_colored, uniform_buffer_layout,
                        mesh_manager.material_manager_map().layout(
                            vw::Model::Material::colored_material_tag));

    vw::MeshRenderer renderer;
    renderer.add_pipeline(vw::Model::Material::textured_material_tag,
                          std::move(textured_pipeline));
    renderer.add_pipeline(vw::Model::Material::colored_material_tag,
                          std::move(colored_pipeline));
    return renderer;
}

class ColorSubpass : public vw::Subpass {
  public:
    ColorSubpass(
        const vw::Device &device, const vw::Model::MeshManager &mesh_manager,
        std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout,
        vw::DescriptorSet descriptor_set,
        std::span<const vk::Format> color_formats, vk::Format depth_format,
        vk::Format)
        : m_device{device}
        , m_mesh_manager{mesh_manager}
        , m_uniform_buffer_layout{uniform_buffer_layout}
        , m_descriptor_set{descriptor_set} {
        m_mesh_renderer =
            create_renderer(m_device, color_formats, depth_format,
                            m_mesh_manager, m_uniform_buffer_layout);
    }

    void execute(vk::CommandBuffer cmd_buffer,
                 vk::Rect2D render_area) const noexcept override {
        vk::Viewport viewport(
            0.0f, 0.0f, static_cast<float>(render_area.extent.width),
            static_cast<float>(render_area.extent.height), 0.0f, 1.0f);
        cmd_buffer.setViewport(0, 1, &viewport);
        cmd_buffer.setScissor(0, 1, &render_area);

        const auto &meshes = m_mesh_manager.meshes();
        auto descriptor_set_handle = m_descriptor_set.handle();
        std::span first_descriptor_sets = {&descriptor_set_handle, 1};
        for (const auto &mesh : meshes) {
            m_mesh_renderer.draw_mesh(cmd_buffer, mesh, first_descriptor_sets);
        }
    }

    AttachmentInfo attachment_information(
        const std::vector<std::shared_ptr<const vw::ImageView>>
            &color_attachments,
        const std::shared_ptr<const vw::ImageView> &depth_attachment)
        const override {
        AttachmentInfo attachments;

        for (const auto &view : color_attachments) {
            attachments.color.push_back(
                vk::RenderingAttachmentInfo()
                    .setImageView(view->handle())
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setClearValue(
                        vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)));
        }

        if (!depth_attachment) {
            throw vw::SubpassNotManagingDepthException(
                std::source_location::current());
        }

        attachments.depth =
            vk::RenderingAttachmentInfo()
                .setImageView(depth_attachment->handle())
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eStore);

        return attachments;
    }

    std::vector<vw::Barrier::ResourceState> resource_states(
        const std::vector<std::shared_ptr<const vw::ImageView>>
            &color_attachments,
        const std::shared_ptr<const vw::ImageView> &depth_attachment)
        const override {
        if (!depth_attachment) {
            throw vw::SubpassNotManagingDepthException(
                std::source_location::current());
        }

        std::vector<vw::Barrier::ResourceState> resources =
            m_descriptor_set.resources();

        for (const auto &view : color_attachments) {
            resources.push_back(vw::Barrier::ImageState{
                .image = view->image()->handle(),
                .subresourceRange = view->subresource_range(),
                .layout = vk::ImageLayout::eColorAttachmentOptimal,
                .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .access = vk::AccessFlagBits2::eColorAttachmentWrite});
        }

        resources.push_back(vw::Barrier::ImageState{
            .image = depth_attachment->image()->handle(),
            .subresourceRange = depth_attachment->subresource_range(),
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                     vk::PipelineStageFlagBits2::eLateFragmentTests,
            .access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite |
                      vk::AccessFlagBits2::eDepthStencilAttachmentRead});

        for (const auto &mesh : m_mesh_manager.meshes()) {
            const auto &material_resources =
                mesh.material().descriptor_set.resources();
            resources.insert(resources.end(), material_resources.begin(),
                             material_resources.end());
        }

        return resources;
    }

  private:
    const vw::Device &m_device;
    const vw::Model::MeshManager &m_mesh_manager;
    std::shared_ptr<const vw::DescriptorSetLayout> m_uniform_buffer_layout;
    vw::MeshRenderer m_mesh_renderer;
    vw::DescriptorSet m_descriptor_set;
};
