#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/Framebuffer.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/Subpass.h"

struct SkyPassTag {};
const auto sky_pass_tag = vw::create_subpass_tag<SkyPassTag>();

class SkyPass : public vw::Subpass {
  public:
    struct UBO {
        glm::mat4 projection;
        glm::mat4 camera;
        float angle = 0.0;
    };

    SkyPass(const vw::Device &device, const vw::Allocator &allocator,
            vw::Width width, vw::Height height, const glm::mat4 &projection,
            const glm::mat4 &camera)
        : m_device{device}
        , m_allocator{allocator}
        , m_width{width}
        , m_height{height} {
        const UBO ubo{projection, camera};
        m_ubo.copy(std::span{&ubo, 1}, 0);

        vw::DescriptorAllocator alloc;
        alloc.add_uniform_buffer(0, m_ubo.handle(), 0, sizeof(UBO));
        m_descriptor_set = m_descriptor_pool.allocate_set(alloc);
    }

    void execute(vk::CommandBuffer cmd_buffer,
                 const vw::Framebuffer &) const noexcept override {
        cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                m_pipeline->handle());
        cmd_buffer.bindDescriptorSets(pipeline_bind_point(),
                                      m_pipeline->layout().handle(), 0,
                                      m_descriptor_set, nullptr);
        cmd_buffer.draw(4, 1, 0, 0);
    }

    const std::vector<vk::AttachmentReference2> &
    color_attachments() const noexcept override {
        static const std::vector<vk::AttachmentReference2> color_attachments = {
            vk::AttachmentReference2(5,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageAspectFlagBits::eColor)};
        return color_attachments;
    }

    const vk::AttachmentReference2 *
    depth_stencil_attachment() const noexcept override {
        static const vk::AttachmentReference2 depth_stencil_attachment =
            vk::AttachmentReference2(
                7, vk::ImageLayout::eDepthStencilAttachmentOptimal,
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

    auto *get_ubo() { return &m_ubo; }

  protected:
    void initialize(const vw::RenderPass &render_pass) override {
        auto vertex = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/quad.spv");
        auto fragment = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/post-process/sky.spv");

        auto pipelineLayout = vw::PipelineLayoutBuilder(m_device)
                                  .with_descriptor_set_layout(m_layout)
                                  .build();

        m_pipeline =
            vw::GraphicsPipelineBuilder(m_device, render_pass, 2,
                                        std::move(pipelineLayout))
                .add_shader(vk::ShaderStageFlagBits::eVertex, std::move(vertex))
                .add_shader(vk::ShaderStageFlagBits::eFragment,
                            std::move(fragment))
                .with_fixed_scissor(int32_t(m_width), int32_t(m_height))
                .with_fixed_viewport(int32_t(m_width), int32_t(m_height))
                .with_topology(vk::PrimitiveTopology::eTriangleStrip)
                .with_depth_test(false, vk::CompareOp::eEqual)
                .add_color_attachment()
                .build();
    }

  private:
    const vw::Device &m_device;
    const vw::Allocator &m_allocator;
    vw::Width m_width;
    vw::Height m_height;
    vw::Buffer<UBO, true, vw::UniformBufferUsage> m_ubo =
        m_allocator.create_buffer<UBO, true, vw::UniformBufferUsage>(1);
    std::shared_ptr<const vw::DescriptorSetLayout> m_layout =
        vw::DescriptorSetLayoutBuilder(m_device)
            .with_uniform_buffer(vk::ShaderStageFlagBits::eFragment, 1)
            .build();

    mutable vw::DescriptorPool m_descriptor_pool =
        vw::DescriptorPoolBuilder(m_device, m_layout).build();
    std::optional<vw::Pipeline> m_pipeline;
    vk::DescriptorSet m_descriptor_set;
};
