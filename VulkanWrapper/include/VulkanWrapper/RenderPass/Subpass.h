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

class ISubpass {
  public:
    ISubpass() noexcept = default;

    ISubpass(const ISubpass &) = delete;
    ISubpass(ISubpass &&) = delete;
    ISubpass &operator=(ISubpass &&) = delete;
    ISubpass &operator=(const ISubpass &) = delete;

    virtual ~ISubpass() = default;

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

  protected:
    virtual void initialize(const IRenderPass &render_pass) = 0;

    template <typename> friend class RenderPass;
};

template <typename RenderPassInformation> class Subpass : public ISubpass {
  public:
    virtual void execute(vk::CommandBuffer cmd_buffer,
                         const RenderPassInformation &render_pass_information)
        const noexcept = 0;
};

} // namespace vw
