#include "VulkanWrapper/RenderPass/ZPass.h"

#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/Vertex.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Model/Scene.h"
#include "VulkanWrapper/RayTracing/RayTracedScene.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"

#include <cassert>
#include <glm/glm.hpp>

namespace vw {

ZPass::ZPass(std::shared_ptr<Device> device,
             std::shared_ptr<Allocator> allocator,
             const std::filesystem::path &shader_dir,
             vk::Format depth_format)
    : RenderPass(device, allocator)
    , m_depth_format(depth_format)
    , m_descriptor_layout(
          DescriptorSetLayoutBuilder(m_device)
              .with_uniform_buffer(
                  vk::ShaderStageFlagBits::eVertex |
                      vk::ShaderStageFlagBits::eFragment,
                  1)
              .build())
    , m_pipeline(
          GraphicsPipelineBuilder(
              m_device,
              PipelineLayoutBuilder(m_device)
                  .with_descriptor_set_layout(m_descriptor_layout)
                  .with_push_constant_range(
                      vk::PushConstantRange()
                          .setStageFlags(
                              vk::ShaderStageFlagBits::eVertex)
                          .setOffset(0)
                          .setSize(sizeof(glm::mat4)))
                  .build())
              .set_depth_format(m_depth_format)
              .add_vertex_binding<Vertex3D>()
              .add_shader(
                  vk::ShaderStageFlagBits::eVertex,
                  ShaderCompiler().compile_file_to_module(
                      m_device,
                      shader_dir / "GBuffer" / "zpass.vert"))
              .with_dynamic_viewport_scissor()
              .with_depth_test(true, vk::CompareOp::eLess)
              .build())
    , m_descriptor_pool(
          DescriptorPoolBuilder(m_device, m_descriptor_layout)
              .build()) {}

std::vector<Slot> ZPass::input_slots() const { return {}; }

std::vector<Slot> ZPass::output_slots() const {
    return {Slot::Depth};
}

void ZPass::set_uniform_buffer(const BufferBase &ubo) {
    m_uniform_buffer = &ubo;
}

void ZPass::set_scene(const rt::RayTracedScene &scene) {
    m_scene = &scene;
}

void ZPass::execute(vk::CommandBuffer cmd,
                    Barrier::ResourceTracker &tracker,
                    Width width, Height height,
                    size_t frame_index) {
    assert(m_uniform_buffer &&
           "ZPass: uniform buffer not set");
    assert(m_scene && "ZPass: scene not set");

    // Lazy allocation of depth image
    const auto &depth = get_or_create_image(
        Slot::Depth, width, height, frame_index, m_depth_format,
        vk::ImageUsageFlagBits::eDepthStencilAttachment |
            vk::ImageUsageFlagBits::eSampled);

    vk::Extent2D extent{static_cast<uint32_t>(width),
                        static_cast<uint32_t>(height)};

    // Create descriptor set with uniform buffer
    DescriptorAllocator descriptor_allocator;
    descriptor_allocator.add_uniform_buffer(
        0, m_uniform_buffer->handle(), 0,
        m_uniform_buffer->size_bytes(),
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

    // Setup depth attachment
    vk::RenderingAttachmentInfo depth_attachment =
        vk::RenderingAttachmentInfo()
            .setImageView(depth.view->handle())
            .setImageLayout(
                vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(
                vk::ClearDepthStencilValue(1.0f, 0));

    vk::RenderingInfo rendering_info =
        vk::RenderingInfo()
            .setRenderArea(vk::Rect2D({0, 0}, extent))
            .setLayerCount(1)
            .setPDepthAttachment(&depth_attachment);

    cmd.beginRendering(rendering_info);

    // Set viewport and scissor
    vk::Viewport viewport(0.0f, 0.0f,
                          static_cast<float>(extent.width),
                          static_cast<float>(extent.height),
                          0.0f, 1.0f);
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
    const auto &scene = m_scene->scene();
    for (const auto &instance : scene.instances()) {
        instance.mesh.draw_zpass(
            cmd, m_pipeline->layout(), instance.transform);
    }

    cmd.endRendering();
}

} // namespace vw
