#pragma once

#include "VulkanWrapper/Descriptors/DescriptorSet.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"

namespace vw {

/**
 * @brief Utility class for fullscreen rendering passes
 *
 * This class provides common functionality for screen-space post-processing passes.
 * It handles viewport/scissor setup, pipeline binding, and fullscreen quad rendering.
 */
class ScreenSpacePass {
  public:
    ScreenSpacePass(std::shared_ptr<const Device> device,
                    std::shared_ptr<const Pipeline> pipeline,
                    DescriptorSet descriptor_set,
                    std::shared_ptr<const ImageView> output_image,
                    std::shared_ptr<const ImageView> depth_image = nullptr);

    void execute(vk::CommandBuffer cmd_buffer) const noexcept;

    struct AttachmentInfo {
        std::vector<vk::RenderingAttachmentInfo> color;
        std::optional<vk::RenderingAttachmentInfo> depth;
        vk::Rect2D render_area;
    };

    AttachmentInfo attachment_information() const;

    std::vector<Barrier::ResourceState> resource_states() const;

  protected:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Pipeline> m_pipeline;
    DescriptorSet m_descriptor_set;
    std::shared_ptr<const ImageView> m_output_image;
    std::shared_ptr<const ImageView> m_depth_image;
};

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
