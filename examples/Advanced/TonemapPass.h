#pragma once

#include "RenderPassInformation.h"
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/Framebuffer.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/Subpass.h"

struct TonemapPassTag {};
const auto tonemap_pass_tag = vw::create_subpass_tag<TonemapPassTag>();

class TonemapPass : public vw::Subpass<TonemapInformation> {
  public:
    TonemapPass(const vw::Device &device, vw::Width width, vw::Height height)
        : m_device{device}
        , m_width{width}
        , m_height{height} {}

    void execute(vk::CommandBuffer cmd_buffer,
                 const TonemapInformation &info) const noexcept override {
        vw::DescriptorAllocator allocator;
        allocator.add_combined_image(0, info.color);
        allocator.add_combined_image(1, info.light); // Sun lighting output
        auto descriptor_set = m_descriptor_pool.allocate_set(allocator);
        cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                m_pipeline->handle());
        cmd_buffer.bindDescriptorSets(pipeline_bind_point(),
                                      m_pipeline->layout().handle(), 0,
                                      descriptor_set, nullptr);
        cmd_buffer.draw(4, 1, 0, 0);
    }

    const std::vector<vk::AttachmentReference2> &
    color_attachments() const noexcept override {
        static const std::vector<vk::AttachmentReference2> color_attachments = {
            vk::AttachmentReference2(0,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageAspectFlagBits::eColor)};
        return color_attachments;
    }

    vw::SubpassDependencyMask input_dependencies() const noexcept override {
        vw::SubpassDependencyMask mask;
        mask.access = vk::AccessFlagBits::eShaderRead;
        mask.stage = vk::PipelineStageFlagBits::eFragmentShader;
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
        auto vertex = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/quad.spv");
        auto fragment = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/post-process/tonemap.spv");

        auto pipelineLayout = vw::PipelineLayoutBuilder(m_device)
                                  .with_descriptor_set_layout(m_layout)
                                  .build();

        m_pipeline =
            vw::GraphicsPipelineBuilder(m_device, render_pass, 0,
                                        std::move(pipelineLayout))
                .add_shader(vk::ShaderStageFlagBits::eVertex, std::move(vertex))
                .add_shader(vk::ShaderStageFlagBits::eFragment,
                            std::move(fragment))
                .with_fixed_scissor(int32_t(m_width), int32_t(m_height))
                .with_fixed_viewport(int32_t(m_width), int32_t(m_height))
                .with_topology(vk::PrimitiveTopology::eTriangleStrip)
                .add_color_attachment()
                .build();
    }

  private:
    const vw::Device &m_device;
    vw::Width m_width;
    vw::Height m_height;
    std::shared_ptr<const vw::DescriptorSetLayout> m_layout =
        vw::DescriptorSetLayoutBuilder(m_device)
            .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
            .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
            .build();
    mutable vw::DescriptorPool m_descriptor_pool =
        vw::DescriptorPoolBuilder(m_device, m_layout).build();
    std::optional<vw::Pipeline> m_pipeline;
};
