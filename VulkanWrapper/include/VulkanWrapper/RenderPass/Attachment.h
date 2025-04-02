#pragma once

namespace vw {

class AttachmentBuilder {
  public:
    AttachmentBuilder &&with_format(vk::Format format) &&;
    AttachmentBuilder &&with_final_layout(vk::ImageLayout layout) &&;
    vk::AttachmentDescription2 build() &&;

  private:
    vk::AttachmentDescription2 m_attachment =
        vk::AttachmentDescription2()
            .setFormat(vk::Format::eUndefined)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eUndefined)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
};

} // namespace vw
