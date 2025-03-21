#include "VulkanWrapper/RenderPass/Subpass.h"

namespace vw {
SubpassBuilder &&
SubpassBuilder::add_color_attachment(const Attachment &attachment,
                                     vk::ImageLayout layout) && {
    m_colorReferences.emplace(attachment, layout);
    return std::move(*this);
}

Subpass SubpassBuilder::build() && { return {std::move(m_colorReferences)}; }
} // namespace vw
