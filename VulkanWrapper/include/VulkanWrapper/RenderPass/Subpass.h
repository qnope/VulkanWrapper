#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/IdentifierTag.h"

namespace vw {

struct SubpassDependencyMask {
    vk::PipelineStageFlags stage = vk::PipelineStageFlagBits::eNone;
    vk::AccessFlags access = vk::AccessFlagBits::eNone;
};

using SubpassTag = IdentifierTag<struct SubpassIdentifierTag>;

template <typename T> SubpassTag create_subpass_tag() {
    return SubpassTag{typeid(T)};
}

class Subpass {
  public:
    Subpass() noexcept = default;

    Subpass(const Subpass &) = delete;
    Subpass(Subpass &&) = delete;
    Subpass &operator=(Subpass &&) = delete;
    Subpass &operator=(const Subpass &) = delete;

    virtual ~Subpass() = default;

    [[nodiscard]] virtual vk::PipelineBindPoint
    pipeline_bind_point() const noexcept;
    [[nodiscard]] virtual const vk::AttachmentReference2 *
    depth_stencil_attachment() const noexcept;

    [[nodiscard]] virtual const std::vector<vk::AttachmentReference2> &
    color_attachments() const noexcept;

    [[nodiscard]] virtual const std::vector<vk::AttachmentReference2> &
    input_attachments() const noexcept;

    [[nodiscard]] virtual SubpassDependencyMask
    input_dependencies() const noexcept = 0;
    [[nodiscard]] virtual SubpassDependencyMask
    output_dependencies() const noexcept = 0;

    virtual void execute(vk::CommandBuffer cmd_buffer,
                         const vw::Framebuffer &framebuffer) const noexcept = 0;

  protected:
    virtual void initialize(const RenderPass &render_pass) = 0;

    friend class RenderPass;
};

} // namespace vw
