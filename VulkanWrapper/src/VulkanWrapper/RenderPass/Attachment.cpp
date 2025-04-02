#include "VulkanWrapper/RenderPass/Attachment.h"

namespace vw {

AttachmentBuilder &&AttachmentBuilder::with_format(vk::Format format) && {
    m_attachment.setFormat(format);
    return std::move(*this);
}

AttachmentBuilder &&
AttachmentBuilder::with_final_layout(vk::ImageLayout finalLayout) && {
    m_attachment.setFinalLayout(finalLayout);
    return std::move(*this);
}

vk::AttachmentDescription2 AttachmentBuilder::build() && {
    return m_attachment;
}
} // namespace vw
