#pragma once

#include "RenderPassInformation.h"
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/DescriptorSet.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/Subpass.h"

inline vw::Pipeline create_zpass_pipeline(
    const vw::Device &device, vk::Format depth_format,
    std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout) {
    auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/GBuffer/zpass.spv");

    auto pipeline_layout =
        vw::PipelineLayoutBuilder(device)
            .with_descriptor_set_layout(uniform_buffer_layout)
            .build();

    return vw::GraphicsPipelineBuilder(device, std::move(pipeline_layout))
        .set_depth_format(depth_format)
        .add_vertex_binding<vw::Vertex3D>()
        .add_shader(vk::ShaderStageFlagBits::eVertex, std::move(vertex_shader))
        .with_dynamic_viewport_scissor()
        .with_depth_test(true, vk::CompareOp::eLess)
        .build();
}

class ZPass : public vw::Subpass {
  public:
    ZPass(const vw::Device &device, const vw::Model::MeshManager &mesh_manager,
          std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout,
          vw::DescriptorSet descriptor_set, std::span<const vk::Format>,
          vk::Format depth_format, vk::Format)
        : m_device{device}
        , m_mesh_manager{mesh_manager}
        , m_uniform_buffer_layout{uniform_buffer_layout}
        , m_descriptor_set{descriptor_set} {
        m_pipeline = create_zpass_pipeline(m_device, depth_format,
                                           m_uniform_buffer_layout);
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
        cmd_buffer.bindPipeline(pipeline_bind_point(), m_pipeline->handle());
        cmd_buffer.bindDescriptorSets(pipeline_bind_point(),
                                      m_pipeline->layout().handle(), 0,
                                      first_descriptor_sets, nullptr);
        for (const auto &mesh : meshes) {
            mesh.draw_zpass(cmd_buffer);
        }
    }

    AttachmentInfo attachment_information(
        const std::vector<std::shared_ptr<const vw::ImageView>> &,
        const std::shared_ptr<const vw::ImageView> &depth_attachment)
        const override {
        AttachmentInfo attachments;

        if (!depth_attachment) {
            throw vw::SubpassNotManagingDepthException(
                std::source_location::current());
        }

        attachments.depth =
            vk::RenderingAttachmentInfo()
                .setImageView(depth_attachment->handle())
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setClearValue(vk::ClearDepthStencilValue(1.0f, 0));

        return attachments;
    }

    std::vector<vw::Barrier::ResourceState> resource_states(
        const std::vector<std::shared_ptr<const vw::ImageView>> &,
        const std::shared_ptr<const vw::ImageView> &depth_attachment)
        const override {
        if (!depth_attachment) {
            throw vw::SubpassNotManagingDepthException(
                std::source_location::current());
        }

        std::vector<vw::Barrier::ResourceState> resources =
            m_descriptor_set.resources();

        resources.push_back(vw::Barrier::ImageState{
            .image = depth_attachment->image()->handle(),
            .subresourceRange = depth_attachment->subresource_range(),
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                     vk::PipelineStageFlagBits2::eLateFragmentTests,
            .access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite |
                      vk::AccessFlagBits2::eDepthStencilAttachmentRead});

        return resources;
    }

  private:
    const vw::Device &m_device;
    const vw::Model::MeshManager &m_mesh_manager;
    std::shared_ptr<const vw::DescriptorSetLayout> m_uniform_buffer_layout;
    vw::DescriptorSet m_descriptor_set;
    std::optional<vw::Pipeline> m_pipeline;
};
