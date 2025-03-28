#pragma once

namespace vw {
struct Attachment {
    std::string id;
    vk::Format format;
    vk::SampleCountFlagBits sampleCount;
    vk::AttachmentLoadOp loadOp;
    vk::AttachmentStoreOp storeOp;
    vk::ImageLayout initialLayout;
    vk::ImageLayout finalLayout;
    vk::ClearValue clearValue;

    std::strong_ordering operator<=>(const Attachment &other) const {
        return id <=> other.id;
    };

    bool operator==(const Attachment &other) const { return id == other.id; }
};

class AttachmentBuilder {
  public:
    AttachmentBuilder(std::string_view id);

    AttachmentBuilder with_format(vk::Format format,
                                  vk::ClearValue clear_value) &&;
    AttachmentBuilder with_final_layout(vk::ImageLayout layout) &&;
    Attachment build() &&;

  private:
    std::string m_id;
    vk::Format m_format = vk::Format::eUndefined;
    vk::SampleCountFlagBits m_sampleCount = vk::SampleCountFlagBits::e1;

    vk::ImageLayout m_initialLayout = vk::ImageLayout::eUndefined;
    vk::ImageLayout m_finalLayout = vk::ImageLayout::eUndefined;
    vk::AttachmentLoadOp m_loadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp m_storeOp = vk::AttachmentStoreOp::eStore;
    vk::ClearValue m_clearValue;
};

} // namespace vw
