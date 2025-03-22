#include "VulkanWrapper/RenderPass/RenderPass.h"

#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

static vk::AttachmentDescription2
attachmentToDescription(const Attachment &attachment) {
    return vk::AttachmentDescription2()
        .setFormat(attachment.format)
        .setSamples(attachment.sampleCount)
        .setLoadOp(attachment.loadOp)
        .setStoreOp(attachment.storeOp)
        .setInitialLayout(attachment.initialLayout)
        .setFinalLayout(attachment.finalLayout)
        .setStencilLoadOp(vk::AttachmentLoadOp::eClear);
}

static vk::AttachmentReference2 computeReference(
    const std::vector<Attachment> &attachments,
    std::pair<const Attachment &, vk::ImageLayout> attachmentAndLayout) {
    auto index = index_of(attachments, attachmentAndLayout.first);
    assert(index);

    return vk::AttachmentReference2().setAttachment(*index).setLayout(
        attachmentAndLayout.second);
}

struct SubpassDescription {
    std::vector<vk::AttachmentReference2> color_attachments;
    std::optional<vk::AttachmentReference2> depth_attachment;
};

vk::SubpassDescription2
toVulkanDescriptions(const SubpassDescription &description) {
    auto result = vk::SubpassDescription2()
                      .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                      .setColorAttachments(description.color_attachments);

    if (description.depth_attachment)
        result.setPDepthStencilAttachment(&*description.depth_attachment);

    return result;
}

static SubpassDescription
subpassToDescription(const std::vector<Attachment> &attachments,
                     const Subpass &subpass) {
    std::vector<vk::AttachmentReference2> color_references =
        subpass.color_attachments |
        std::views::transform(std::bind_front(computeReference, attachments)) |
        to<std::vector>;

    SubpassDescription result{.color_attachments = std::move(color_references)};

    if (subpass.depth_attachment) {
        result.depth_attachment =
            computeReference(attachments, *subpass.depth_attachment);
    }

    return result;
}

RenderPassBuilder::RenderPassBuilder(const Device &device)
    : m_device{&device} {}

RenderPassBuilder &&RenderPassBuilder::add_subpass(Subpass subpass) && {
    m_subpasses.push_back(std::move(subpass));
    return std::move(*this);
}

RenderPass RenderPassBuilder::build() && {

    const auto attachments = create_attachments();
    const auto attachmentDescriptions =
        attachments | std::views::transform(attachmentToDescription) |
        to<std::vector>;

    const auto toSubpassDescriptions =
        std::bind_front(subpassToDescription, attachments);
    const auto subpassDescriptions =
        m_subpasses | std::views::transform(toSubpassDescriptions) |
        to<std::vector>;
    const auto vkSubpassDescriptions =
        subpassDescriptions | std::views::transform(toVulkanDescriptions) |
        to<std::vector>;

    const auto info = vk::RenderPassCreateInfo2()
                          .setAttachments(attachmentDescriptions)
                          .setSubpasses(vkSubpassDescriptions);

    auto [result, renderPass] =
        m_device->handle().createRenderPass2Unique(info);

    if (result != vk::Result::eSuccess) {
        throw RenderPassCreationException{std::source_location::current()};
    }
    return RenderPass{std::move(renderPass)};
}

std::vector<Attachment> RenderPassBuilder::create_attachments() const noexcept {
    std::set<Attachment> attachments;

    for (const auto &subpass : m_subpasses) {
        for (const auto &attachment :
             subpass.color_attachments | std::views::keys) {
            attachments.insert(attachment);
        }
        if (subpass.depth_attachment)
            attachments.insert(subpass.depth_attachment->first);
    }

    return attachments | to<std::vector>;
}

} // namespace vw
