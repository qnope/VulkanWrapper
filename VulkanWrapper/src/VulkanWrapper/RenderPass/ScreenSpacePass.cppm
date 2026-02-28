export module vw.renderpass:screen_space_pass;
import std3rd;
import vulkan3rd;
import :subpass;
import vw.descriptors;
import vw.vulkan;
import vw.memory;
import vw.pipeline;

export namespace vw {

/**
 * @brief Base class for screen-space rendering passes
 *
 * @tparam SlotEnum Enumeration defining the image slots for this pass
 *
 * This class provides common functionality for fullscreen post-processing
 * passes:
 * - Inherits lazy image allocation from Subpass
 * - Provides helper to create default sampler
 * - Provides helper to render fullscreen quad with proper setup
 */
template <typename SlotEnum> class ScreenSpacePass : public Subpass<SlotEnum> {
  public:
    ScreenSpacePass(std::shared_ptr<Device> device,
                    std::shared_ptr<Allocator> allocator)
        : Subpass<SlotEnum>(std::move(device), std::move(allocator)) {}

  protected:
    /**
     * @brief Create a default sampler with linear filtering
     */
    std::shared_ptr<const Sampler> create_default_sampler() {
        return SamplerBuilder(this->m_device).build();
    }

    /**
     * @brief Render a fullscreen quad with the given parameters
     */
    void render_fullscreen(
        vk::CommandBuffer cmd, vk::Extent2D extent,
        const vk::RenderingAttachmentInfo &color_attachment,
        const vk::RenderingAttachmentInfo *depth_attachment,
        const Pipeline &pipeline,
        std::optional<DescriptorSet> descriptor_set = std::nullopt,
        const void *push_constants = nullptr, size_t push_constants_size = 0) {

        vk::RenderingInfo rendering_info =
            vk::RenderingInfo()
                .setRenderArea(vk::Rect2D({0, 0}, extent))
                .setLayerCount(1)
                .setColorAttachments(color_attachment);

        if (depth_attachment) {
            rendering_info.setPDepthAttachment(depth_attachment);
        }

        cmd.beginRendering(rendering_info);

        vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width),
                              static_cast<float>(extent.height), 0.0f, 1.0f);
        vk::Rect2D scissor({0, 0}, extent);
        cmd.setViewport(0, 1, &viewport);
        cmd.setScissor(0, 1, &scissor);

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle());

        if (descriptor_set) {
            auto descriptor_handle = descriptor_set->handle();
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   pipeline.layout().handle(), 0, 1,
                                   &descriptor_handle, 0, nullptr);
        }

        if (push_constants && push_constants_size > 0) {
            cmd.pushConstants(pipeline.layout().handle(),
                              vk::ShaderStageFlagBits::eFragment, 0,
                              push_constants_size, push_constants);
        }

        cmd.draw(4, 1, 0, 0);

        cmd.endRendering();
    }
};

/**
 * @brief Create a graphics pipeline for screen-space rendering
 */
std::shared_ptr<const Pipeline> create_screen_space_pipeline(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const ShaderModule> vertex_shader,
    std::shared_ptr<const ShaderModule> fragment_shader,
    std::shared_ptr<const DescriptorSetLayout> descriptor_set_layout,
    vk::Format color_format, vk::Format depth_format = vk::Format::eUndefined,
    vk::CompareOp depth_compare_op = vk::CompareOp::eAlways,
    std::vector<vk::PushConstantRange> push_constants = {},
    std::optional<ColorBlendConfig> blend = std::nullopt);

} // namespace vw
