#pragma once

#include "VulkanWrapper/Utils/IdentifierTag.h"

namespace vw {
using AttachmentTag = IdentifierTag<struct AttachmentIdentifierTag>;

template <typename T> AttachmentTag create_attachment_tag() {
    return AttachmentTag{typeid(T)};
}

struct Attachment {
    AttachmentTag id;
    vk::Format format;
    vk::SampleCountFlagBits sampleCount;
    vk::AttachmentLoadOp loadOp;
    vk::AttachmentStoreOp storeOp;
    vk::ImageLayout initialLayout;
    vk::ImageLayout finalLayout;
    vk::ClearValue clearValue;

    bool operator==(const Attachment &other) const { return id == other.id; }
    std::strong_ordering operator<=>(const Attachment &other) const {
        return id <=> other.id;
    }
};

class AttachmentBuilder {
  public:
    AttachmentBuilder(AttachmentTag id);

    AttachmentBuilder with_format(vk::Format format,
                                  vk::ClearValue clear_value) &&;
    AttachmentBuilder with_final_layout(vk::ImageLayout layout) &&;
    Attachment build() &&;

  private:
    AttachmentTag m_id;
    vk::Format m_format = vk::Format::eUndefined;
    vk::SampleCountFlagBits m_sampleCount = vk::SampleCountFlagBits::e1;

    vk::ImageLayout m_initialLayout = vk::ImageLayout::eUndefined;
    vk::ImageLayout m_finalLayout = vk::ImageLayout::eUndefined;
    vk::AttachmentLoadOp m_loadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp m_storeOp = vk::AttachmentStoreOp::eStore;
    vk::ClearValue m_clearValue;
};

} // namespace vw

template <> struct std::hash<vw::Attachment> {
    std::size_t operator()(const vw::Attachment &x) const noexcept {
        return std::hash<vw::AttachmentTag>{}(x.id);
    }
};
