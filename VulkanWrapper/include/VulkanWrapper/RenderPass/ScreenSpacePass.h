#pragma once

#include "VulkanWrapper/Descriptors/DescriptorSet.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"
#include <optional>

namespace vw {

/**
 * @brief Base class for screen-space rendering passes
 *
 * This class provides common functionality for fullscreen
 * post-processing passes:
 * - Inherits lazy image allocation from RenderPass
 * - Provides helper to create default sampler
 * - Provides helper to render fullscreen quad with proper setup
 *
 * Derived passes should:
 * 1. Implement input_slots(), output_slots(), execute(), name()
 * 2. Create their own pipeline and descriptor pool in constructor
 * 3. Use render_fullscreen() in execute() to eliminate boilerplate
 */
class ScreenSpacePass : public RenderPass {
  public:
    using RenderPass::RenderPass; // inherit constructor

  protected:
    /**
     * @brief Create a default sampler with linear filtering
     *
     * Creates a sampler suitable for most screen-space
     * post-processing needs. Uses linear filtering and
     * clamp-to-edge addressing.
     *
     * @return Shared pointer to the created sampler
     */
    std::shared_ptr<const Sampler> create_default_sampler() {
        return SamplerBuilder(m_device).build();
    }

    /**
     * @brief Render a fullscreen quad with the given parameters
     *
     * This method handles all the common rendering boilerplate:
     * - Setting up vk::RenderingInfo with color and optional depth
     * - Beginning rendering
     * - Setting viewport and scissor
     * - Binding pipeline and descriptor sets (if provided)
     * - Pushing constants (if provided)
     * - Drawing fullscreen quad (4 vertices, triangle strip)
     * - Ending rendering
     */
    void render_fullscreen(
        vk::CommandBuffer cmd, vk::Extent2D extent,
        const vk::RenderingAttachmentInfo &color_attachment,
        const vk::RenderingAttachmentInfo *depth_attachment,
        const Pipeline &pipeline,
        std::optional<DescriptorSet> descriptor_set = std::nullopt,
        const void *push_constants = nullptr,
        size_t push_constants_size = 0) {

        vk::RenderingInfo rendering_info =
            vk::RenderingInfo()
                .setRenderArea(vk::Rect2D({0, 0}, extent))
                .setLayerCount(1)
                .setColorAttachments(color_attachment);

        if (depth_attachment) {
            rendering_info.setPDepthAttachment(depth_attachment);
        }

        cmd.beginRendering(rendering_info);

        vk::Viewport viewport(
            0.0f, 0.0f, static_cast<float>(extent.width),
            static_cast<float>(extent.height), 0.0f, 1.0f);
        vk::Rect2D scissor({0, 0}, extent);
        cmd.setViewport(0, 1, &viewport);
        cmd.setScissor(0, 1, &scissor);

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                         pipeline.handle());

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
 *
 * Creates a pipeline with:
 * - Triangle strip topology (for fullscreen quad)
 * - Dynamic viewport and scissor
 * - Single color attachment with optional blending
 * - Optional depth testing
 */
std::shared_ptr<const Pipeline> create_screen_space_pipeline(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const ShaderModule> vertex_shader,
    std::shared_ptr<const ShaderModule> fragment_shader,
    std::shared_ptr<const DescriptorSetLayout> descriptor_set_layout,
    vk::Format color_format,
    vk::Format depth_format = vk::Format::eUndefined,
    vk::CompareOp depth_compare_op = vk::CompareOp::eAlways,
    std::vector<vk::PushConstantRange> push_constants = {},
    std::optional<ColorBlendConfig> blend = std::nullopt);

} // namespace vw
