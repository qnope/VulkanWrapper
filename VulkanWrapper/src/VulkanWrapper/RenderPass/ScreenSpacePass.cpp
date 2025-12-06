#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"

#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

ScreenSpacePass::ScreenSpacePass(std::shared_ptr<const Device> device,
                                 std::shared_ptr<const Pipeline> pipeline,
                                 DescriptorSet descriptor_set,
                                 std::shared_ptr<const ImageView> output_image,
                                 std::shared_ptr<const ImageView> depth_image)
    : m_device(std::move(device))
    , m_pipeline(std::move(pipeline))
    , m_descriptor_set(std::move(descriptor_set))
    , m_output_image(std::move(output_image))
    , m_depth_image(std::move(depth_image)) {}

void ScreenSpacePass::execute(vk::CommandBuffer cmd_buffer) const noexcept {
    vk::Rect2D render_area;
    render_area.extent = m_output_image->image()->extent2D();

    vk::Viewport viewport(
        0.0f, 0.0f, static_cast<float>(render_area.extent.width),
        static_cast<float>(render_area.extent.height), 0.0f, 1.0f);

    cmd_buffer.setViewport(0, 1, &viewport);
    cmd_buffer.setScissor(0, 1, &render_area);

    cmd_buffer.bindPipeline(pipeline_bind_point(), m_pipeline->handle());

    auto descriptor_set_handle = m_descriptor_set.handle();
    cmd_buffer.bindDescriptorSets(pipeline_bind_point(),
                                  m_pipeline->layout().handle(), 0, 1,
                                  &descriptor_set_handle, 0, nullptr);

    cmd_buffer.draw(4, 1, 0, 0);
}

Subpass::AttachmentInfo ScreenSpacePass::attachment_information() const {
    AttachmentInfo attachments;

    attachments.color.push_back(
        vk::RenderingAttachmentInfo()
            .setImageView(m_output_image->handle())
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eLoad)
            .setStoreOp(vk::AttachmentStoreOp::eStore));

    if (m_depth_image) {
        attachments.depth =
            vk::RenderingAttachmentInfo()
                .setImageView(m_depth_image->handle())
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eStore);
    }

    attachments.render_area.extent = m_output_image->image()->extent2D();
    return attachments;
}

std::vector<Barrier::ResourceState> ScreenSpacePass::resource_states() const {
    std::vector<Barrier::ResourceState> resources =
        m_descriptor_set.resources();

    resources.push_back(Barrier::ImageState{
        .image = m_output_image->image()->handle(),
        .subresourceRange = m_output_image->subresource_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite |
                  vk::AccessFlagBits2::eColorAttachmentRead});

    if (m_depth_image) {
        resources.push_back(Barrier::ImageState{
            .image = m_depth_image->image()->handle(),
            .subresourceRange = m_depth_image->subresource_range(),
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                     vk::PipelineStageFlagBits2::eLateFragmentTests,
            .access = vk::AccessFlagBits2::eDepthStencilAttachmentRead});
    }

    return resources;
}

std::shared_ptr<const Pipeline> create_screen_space_pipeline(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const ShaderModule> vertex_shader,
    std::shared_ptr<const ShaderModule> fragment_shader,
    std::shared_ptr<const DescriptorSetLayout> descriptor_set_layout,
    vk::Format color_format, vk::Format depth_format, bool depth_test,
    vk::CompareOp depth_compare_op,
    std::vector<vk::PushConstantRange> push_constants) {

    auto pipeline_layout_builder =
        PipelineLayoutBuilder(device).with_descriptor_set_layout(
            descriptor_set_layout);

    for (const auto &pc : push_constants) {
        std::move(pipeline_layout_builder).with_push_constant_range(pc);
    }

    auto pipeline_layout = std::move(pipeline_layout_builder).build();

    auto builder = GraphicsPipelineBuilder(device, std::move(pipeline_layout))
                       .add_shader(vk::ShaderStageFlagBits::eVertex,
                                   std::move(vertex_shader))
                       .add_shader(vk::ShaderStageFlagBits::eFragment,
                                   std::move(fragment_shader))
                       .with_dynamic_viewport_scissor()
                       .with_topology(vk::PrimitiveTopology::eTriangleStrip)
                       .add_color_attachment(color_format);

    if (depth_format != vk::Format::eUndefined) {
        std::move(builder)
            .set_depth_format(depth_format)
            .with_depth_test(false, depth_compare_op);
    }

    return std::move(builder).build();
}

} // namespace vw
