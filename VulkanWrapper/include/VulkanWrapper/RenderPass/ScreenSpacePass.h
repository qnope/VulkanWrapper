#pragma once

#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/Descriptors/DescriptorSet.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"

namespace vw {

/**
 * @brief Base class for screen-space rendering passes
 *
 * @tparam SlotEnum Enumeration defining the image slots for this pass
 *
 * This class provides common functionality for fullscreen post-processing passes:
 * - Inherits lazy image allocation from Subpass
 * - Provides helper to create default sampler
 * - Provides helper to render fullscreen quad with proper setup
 *
 * Derived passes should:
 * 1. Define their own SlotEnum for image allocation (or use empty enum if no allocation needed)
 * 2. Create their own pipeline and descriptor pool in constructor
 * 3. Use render_fullscreen() in execute() to eliminate boilerplate
 */
template <typename SlotEnum>
class ScreenSpacePass : public Subpass<SlotEnum> {
  public:
    ScreenSpacePass(std::shared_ptr<Device> device,
                    std::shared_ptr<Allocator> allocator)
        : Subpass<SlotEnum>(std::move(device), std::move(allocator)) {}

  protected:
    /**
     * @brief Create a default sampler with linear filtering
     *
     * Creates a sampler suitable for most screen-space post-processing needs.
     * Uses linear filtering and clamp-to-edge addressing.
     *
     * @return Shared pointer to the created sampler
     */
    std::shared_ptr<const Sampler> create_default_sampler() {
        return SamplerBuilder(this->m_device).build();
    }

    /**
     * @brief Render a fullscreen quad with the given parameters
     *
     * This method handles all the common rendering boilerplate:
     * - Setting up vk::RenderingInfo with color and optional depth attachment
     * - Beginning rendering
     * - Setting viewport and scissor
     * - Binding pipeline and descriptor sets
     * - Pushing constants
     * - Drawing fullscreen quad (4 vertices, triangle strip)
     * - Ending rendering
     *
     * @tparam PushConstantsType Type of the push constants structure
     * @param cmd Command buffer to record into
     * @param extent Render extent (width, height)
     * @param color_attachment Color attachment info (setup by caller with load/clear ops)
     * @param depth_attachment Optional depth attachment info (nullptr if none)
     * @param pipeline Pipeline to bind
     * @param descriptor_set Descriptor set to bind
     * @param push_constants Push constants to push
     */
    template <typename PushConstantsType>
    void render_fullscreen(vk::CommandBuffer cmd, vk::Extent2D extent,
                           const vk::RenderingAttachmentInfo &color_attachment,
                           const vk::RenderingAttachmentInfo *depth_attachment,
                           const Pipeline &pipeline,
                           const DescriptorSet &descriptor_set,
                           const PushConstantsType &push_constants) {

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

        auto descriptor_handle = descriptor_set.handle();
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               pipeline.layout().handle(), 0, 1,
                               &descriptor_handle, 0, nullptr);

        cmd.pushConstants(pipeline.layout().handle(),
                          vk::ShaderStageFlagBits::eFragment, 0,
                          sizeof(PushConstantsType), &push_constants);

        cmd.draw(4, 1, 0, 0);

        cmd.endRendering();
    }

    /**
     * @brief Render a fullscreen quad without push constants
     *
     * Overload for passes that don't use push constants.
     *
     * @param cmd Command buffer to record into
     * @param extent Render extent (width, height)
     * @param color_attachment Color attachment info
     * @param depth_attachment Optional depth attachment info (nullptr if none)
     * @param pipeline Pipeline to bind
     * @param descriptor_set Descriptor set to bind
     */
    void render_fullscreen(vk::CommandBuffer cmd, vk::Extent2D extent,
                           const vk::RenderingAttachmentInfo &color_attachment,
                           const vk::RenderingAttachmentInfo *depth_attachment,
                           const Pipeline &pipeline,
                           const DescriptorSet &descriptor_set) {

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

        auto descriptor_handle = descriptor_set.handle();
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               pipeline.layout().handle(), 0, 1,
                               &descriptor_handle, 0, nullptr);

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
 * - Single color attachment
 * - Optional depth testing
 *
 * @param device Device to create pipeline on
 * @param vertex_shader Vertex shader module
 * @param fragment_shader Fragment shader module
 * @param descriptor_set_layout Descriptor set layout
 * @param color_format Format of the color attachment
 * @param depth_format Format of depth attachment (eUndefined if no depth)
 * @param depth_test Enable depth testing
 * @param depth_compare_op Depth comparison operator
 * @param push_constants Push constant ranges
 * @return Shared pointer to created pipeline
 */
std::shared_ptr<const Pipeline> create_screen_space_pipeline(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const ShaderModule> vertex_shader,
    std::shared_ptr<const ShaderModule> fragment_shader,
    std::shared_ptr<const DescriptorSetLayout> descriptor_set_layout,
    vk::Format color_format, vk::Format depth_format = vk::Format::eUndefined,
    bool depth_test = false,
    vk::CompareOp depth_compare_op = vk::CompareOp::eAlways,
    std::vector<vk::PushConstantRange> push_constants = {});

} // namespace vw
