#include "VulkanWrapper/RenderPass/Subpass.h"

namespace vw {
SubpassBuilder &&
SubpassBuilder::add_color_attachment(const Attachment &attachment,
                                     vk::ImageLayout layout) && {
    m_color_attachments.emplace(attachment, layout);
    return std::move(*this);
}

SubpassBuilder &&
SubpassBuilder::add_depth_stencil_attachment(const Attachment &attachment) && {
    m_depth_attachment.emplace(attachment,
                               vk::ImageLayout::eDepthStencilAttachmentOptimal);
    return std::move(*this);
}

Subpass SubpassBuilder::build() && {
    return {.color_attachments = std::move(m_color_attachments),
            .depth_attachment = std::move(m_depth_attachment)};
}
} // namespace vw
