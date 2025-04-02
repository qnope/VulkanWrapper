#pragma once

#include "VulkanWrapper/fwd.h"

namespace vw {

class Subpass {
  public:
    virtual ~Subpass() = default;

    virtual vk::PipelineBindPoint pipeline_bind_point() const noexcept;
    virtual const vk::AttachmentReference2 *
    depth_stencil_attachment() const noexcept;

    virtual const std::vector<vk::AttachmentReference2> &
    color_attachments() const noexcept = 0;

    virtual void execute(vk::CommandBuffer cmd_buffer,
                         std::span<const vk::DescriptorSet>
                             first_descriptor_sets) const noexcept = 0;

  protected:
    virtual void initialize(const RenderPass &render_pass) = 0;

    friend class RenderPass;
};

} // namespace vw
