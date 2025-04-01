#pragma once

#include "VulkanWrapper/RenderPass/Attachment.h"

namespace vw {

struct Subpass {
    std::unordered_map<Attachment, vk::ImageLayout> color_attachments;
    std::optional<std::pair<Attachment, vk::ImageLayout>> depth_attachment;
};

class SubpassBuilder {
  public:
    SubpassBuilder &&add_color_attachment(const Attachment &attachment,
                                          vk::ImageLayout layout) &&;
    SubpassBuilder &&
    add_depth_stencil_attachment(const Attachment &attachment) &&;
    Subpass build() &&;

  private:
    std::unordered_map<Attachment, vk::ImageLayout> m_color_attachments;
    std::optional<std::pair<Attachment, vk::ImageLayout>> m_depth_attachment;
};
} // namespace vw
