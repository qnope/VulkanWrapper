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
        .setFinalLayout(attachment.finalLayout);
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
    vk::PipelineBindPoint bindingPoint;
    std::vector<vk::AttachmentReference2> colorAttachments;
};

vk::SubpassDescription2
toVulkanDescriptions(const SubpassDescription &description) {
    return vk::SubpassDescription2()
        .setPipelineBindPoint(description.bindingPoint)
        .setColorAttachments(description.colorAttachments);
}

static SubpassDescription subpassToDescription(
    const std::vector<Attachment> &attachments,
    const std::pair<vk::PipelineBindPoint, Subpass> &bindingPointAndSubpass) {

    std::vector<vk::AttachmentReference2> colorAttachments =
        bindingPointAndSubpass.second.colorReferences |
        std::views::transform(std::bind_front(computeReference, attachments)) |
        to<std::vector>;

    return {bindingPointAndSubpass.first, std::move(colorAttachments)};
}

RenderPassBuilder::RenderPassBuilder(const Device &device)
    : m_device{device} {}

RenderPassBuilder &&RenderPassBuilder::add_subpass(Subpass subpass) && {
    m_subpasses.emplace_back(vk::PipelineBindPoint::eGraphics,
                             std::move(subpass));
    return std::move(*this);
}

RenderPass RenderPassBuilder::build() && {

    const auto attachments = createAttachments();
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

    auto [result, renderPass] = m_device.handle().createRenderPass2Unique(info);

    if (result != vk::Result::eSuccess)
        throw RenderPassCreationException{std::source_location::current()};
    return RenderPass{std::move(renderPass)};
}

std::vector<Attachment> RenderPassBuilder::createAttachments() const noexcept {
    std::set<Attachment> attachments;

    for (const auto &subpass : m_subpasses | std::views::values)
        attachments.insert_range(subpass.colorReferences | std::views::keys);

    return attachments | to<std::vector>;
}

} // namespace vw
