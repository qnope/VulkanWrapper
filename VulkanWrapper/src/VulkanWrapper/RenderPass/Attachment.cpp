#include "VulkanWrapper/RenderPass/Attachment.h"

namespace vw {
AttachmentBuilder::AttachmentBuilder(AttachmentTag id)
    : m_id{id} {}

AttachmentBuilder
AttachmentBuilder::with_format(vk::Format format,
                               vk::ClearValue clear_value) && {
    m_format = format;
    m_clearValue = clear_value;
    return std::move(*this);
}

AttachmentBuilder
AttachmentBuilder::with_final_layout(vk::ImageLayout finalLayout) && {
    m_finalLayout = finalLayout;
    return std::move(*this);
}

Attachment AttachmentBuilder::build() && {
    return Attachment{.id = std::move(m_id),
                      .format = m_format,
                      .sampleCount = m_sampleCount,
                      .loadOp = m_loadOp,
                      .storeOp = m_storeOp,
                      .initialLayout = m_initialLayout,
                      .finalLayout = m_finalLayout,
                      .clearValue = m_clearValue};
}
} // namespace vw
