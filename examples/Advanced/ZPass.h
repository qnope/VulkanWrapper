#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSet.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/Model/Scene.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"

enum class ZPassSlot { Depth };

/**
 * @brief Functional depth pre-pass (Z-Pass) with lazy image allocation
 *
 * This pass lazily allocates its depth image on first execute() call.
 * Images are cached by (width, height, frame_index) and reused on subsequent
 * calls.
 */
class ZPass : public vw::Subpass<ZPassSlot> {
  public:
    ZPass(std::shared_ptr<vw::Device> device,
          std::shared_ptr<vw::Allocator> allocator,
          vk::Format depth_format = vk::Format::eD32Sfloat)
        : Subpass(device, allocator)
        , m_depth_format(depth_format)
        , m_descriptor_pool(create_descriptor_pool()) {}

    /**
     * @brief Execute the depth pre-pass
     *
     * @param cmd Command buffer to record into
     * @param tracker Resource tracker for barrier management
     * @param width Render width
     * @param height Render height
     * @param frame_index Frame index for multi-buffering
     * @param scene Scene containing mesh instances to render
     * @param uniform_buffer Uniform buffer containing view/projection matrices
     * @return The output depth image view
     */
    template <typename UBOType>
    std::shared_ptr<const vw::ImageView>
    execute(vk::CommandBuffer cmd, vw::Barrier::ResourceTracker &tracker,
            vw::Width width, vw::Height height, size_t frame_index,
            const vw::Model::Scene &scene,
            const vw::Buffer<UBOType, true, vw::UniformBufferUsage>
                &uniform_buffer) {

        // Lazy allocation of depth image
        const auto &depth = get_or_create_image(
            ZPassSlot::Depth, width, height, frame_index, m_depth_format,
            vk::ImageUsageFlagBits::eDepthStencilAttachment |
                vk::ImageUsageFlagBits::eSampled);

        vk::Extent2D extent{static_cast<uint32_t>(width),
                            static_cast<uint32_t>(height)};

        // Create descriptor set with uniform buffer
        vw::DescriptorAllocator descriptor_allocator;
        descriptor_allocator.add_uniform_buffer(
            0, uniform_buffer.handle(), 0, uniform_buffer.size_bytes(),
            vk::PipelineStageFlagBits2::eVertexShader,
            vk::AccessFlagBits2::eUniformRead);

        auto descriptor_set =
            m_descriptor_pool.allocate_set(descriptor_allocator);

        // Request resource states for barriers
        for (const auto &resource : descriptor_set.resources()) {
            tracker.request(resource);
        }

        // Flush barriers
        tracker.flush(cmd);

        // Setup rendering
        vk::RenderingAttachmentInfo depth_attachment =
            vk::RenderingAttachmentInfo()
                .setImageView(depth.view->handle())
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setClearValue(vk::ClearDepthStencilValue(1.0f, 0));

        vk::RenderingInfo rendering_info =
            vk::RenderingInfo()
                .setRenderArea(vk::Rect2D({0, 0}, extent))
                .setLayerCount(1)
                .setPDepthAttachment(&depth_attachment);

        cmd.beginRendering(rendering_info);

        // Set viewport and scissor
        vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width),
                              static_cast<float>(extent.height), 0.0f, 1.0f);
        vk::Rect2D scissor({0, 0}, extent);
        cmd.setViewport(0, 1, &viewport);
        cmd.setScissor(0, 1, &scissor);

        // Bind pipeline and descriptors
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                         m_pipeline->handle());

        auto descriptor_handle = descriptor_set.handle();
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               m_pipeline->layout().handle(), 0, 1,
                               &descriptor_handle, 0, nullptr);

        // Draw all mesh instances
        for (const auto &instance : scene.instances()) {
            instance.mesh.draw_zpass(cmd, m_pipeline->layout(),
                                     instance.transform);
        }

        cmd.endRendering();

        return depth.view;
    }

  private:
    vw::DescriptorPool create_descriptor_pool() {
        // Create descriptor layout for uniform buffer
        m_descriptor_layout =
            vw::DescriptorSetLayoutBuilder(m_device)
                .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex |
                                         vk::ShaderStageFlagBits::eFragment,
                                     1)
                .build();

        // Create pipeline
        auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/GBuffer/zpass.spv");

        auto pipeline_layout =
            vw::PipelineLayoutBuilder(m_device)
                .with_descriptor_set_layout(m_descriptor_layout)
                .with_push_constant_range(
                    vk::PushConstantRange()
                        .setStageFlags(vk::ShaderStageFlagBits::eVertex)
                        .setOffset(0)
                        .setSize(sizeof(glm::mat4)))
                .build();

        m_pipeline =
            vw::GraphicsPipelineBuilder(m_device, std::move(pipeline_layout))
                .set_depth_format(m_depth_format)
                .add_vertex_binding<vw::Vertex3D>()
                .add_shader(vk::ShaderStageFlagBits::eVertex,
                            std::move(vertex_shader))
                .with_dynamic_viewport_scissor()
                .with_depth_test(true, vk::CompareOp::eLess)
                .build();

        return vw::DescriptorPoolBuilder(m_device, m_descriptor_layout).build();
    }

    vk::Format m_depth_format;

    // Resources (order matters! m_pipeline must be before m_descriptor_pool)
    std::shared_ptr<vw::DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<const vw::Pipeline> m_pipeline;
    vw::DescriptorPool m_descriptor_pool;
};
