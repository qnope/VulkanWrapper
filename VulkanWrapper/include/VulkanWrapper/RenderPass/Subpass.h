#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/IdentifierTag.h"
#include <vulkan/vulkan.hpp>
#include <span>

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

  protected:
    virtual void initialize(std::span<const vk::Format> color_attachment_formats,
                            vk::Format depth_attachment_format,
                            vk::Format stencil_attachment_format) = 0;
};

template <typename RenderPassInformation> class Subpass : public ISubpass {
  public:
    virtual void execute(vk::CommandBuffer cmd_buffer,
                         const RenderPassInformation &render_pass_information)
        const noexcept = 0;
};

} // namespace vw
