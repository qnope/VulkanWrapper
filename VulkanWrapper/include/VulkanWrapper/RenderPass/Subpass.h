#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/IdentifierTag.h"
#include <span>
#include <vulkan/vulkan.hpp>

namespace vw {

using SubpassNotManagingDepthException =
    TaggedException<struct SubpassNotManagingDepthTag>;

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

    virtual void execute(vk::CommandBuffer cmd_buffer) const noexcept = 0;

    virtual std::vector<vk::RenderingAttachmentInfo>
    color_attachment_information() const noexcept = 0;

    virtual vk::RenderingAttachmentInfo depth_attachment_information() const;
};

} // namespace vw
