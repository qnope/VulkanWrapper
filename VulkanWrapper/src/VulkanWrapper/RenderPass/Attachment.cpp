#include "VulkanWrapper/RenderPass/Attachment.h"

namespace vw {
AttachmentBuilder::AttachmentBuilder(std::string_view id)
    : m_id{id} {}

AttachmentBuilder AttachmentBuilder::withFormat(vk::Format format) && {
    m_format = format;
    return std::move(*this);
}

AttachmentBuilder
AttachmentBuilder::withFinalLayout(vk::ImageLayout finalLayout) && {
    m_finalLayout = finalLayout;
    return std::move(*this);
}

Attachment AttachmentBuilder::build() && {
    return Attachment{.id = m_id,
                      .format = m_format,
                      .sampleCount = m_sampleCount,
                      .loadOp = m_loadOp,
                      .storeOp = m_storeOp,
                      .initialLayout = m_initialLayout,
                      .finalLayout = m_finalLayout};
}
} // namespace vw
