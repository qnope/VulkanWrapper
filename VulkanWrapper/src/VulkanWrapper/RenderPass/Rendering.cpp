#include "VulkanWrapper/RenderPass/Rendering.h"

#include "VulkanWrapper/Memory/Barrier.h"

namespace vw {

Rendering::Rendering(std::vector<SubpassInfo> subpasses)
    : m_subpasses(std::move(subpasses)) {}

void Rendering::execute(vk::CommandBuffer cmd_buffer,
                        Barrier::ResourceTracker &resource_tracker) const {
    for (const auto &subpassInfo : m_subpasses) {
        auto attachmentInfos =
            subpassInfo.subpass->color_attachment_information();

        vk::Rect2D renderArea;
        if (!subpassInfo.color_attachments.empty()) {
            renderArea.extent =
                subpassInfo.color_attachments.front()->image()->extent2D();
        } else if (subpassInfo.depth_attachment) {
            renderArea.extent =
                subpassInfo.depth_attachment->image()->extent2D();
        }

        // Set image views for color attachments
        size_t colorIndex = 0;
        for (auto &attachmentInfo : attachmentInfos) {
            if (colorIndex < subpassInfo.color_attachments.size()) {
                const auto &view = subpassInfo.color_attachments[colorIndex];

                resource_tracker.request(Barrier::ImageState{
                    .image = view->image()->handle(),
                    .subresourceRange = view->subresource_range(),
                    .layout = attachmentInfo.imageLayout,
                    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                    .access = vk::AccessFlagBits2::eColorAttachmentWrite});

                attachmentInfo.setImageView(view->handle());
                colorIndex++;
            }
        }

        auto renderingInfo = vk::RenderingInfo()
                                 .setRenderArea(renderArea)
                                 .setLayerCount(1)
                                 .setColorAttachments(attachmentInfos);

        vk::RenderingAttachmentInfo depthInfo;

        if (subpassInfo.depth_attachment) {
            depthInfo = subpassInfo.subpass->depth_attachment_information();

            resource_tracker.request(Barrier::ImageState{
                .image = subpassInfo.depth_attachment->image()->handle(),
                .subresourceRange =
                    subpassInfo.depth_attachment->subresource_range(),
                .layout = depthInfo.imageLayout,
                .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                         vk::PipelineStageFlagBits2::eLateFragmentTests,
                .access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite |
                          vk::AccessFlagBits2::eDepthStencilAttachmentRead});

            depthInfo.setImageView(subpassInfo.depth_attachment->handle());
            renderingInfo.setPDepthAttachment(&depthInfo);
        }

        for (const auto &resource : subpassInfo.subpass->resource_states()) {
            resource_tracker.request(resource);
        }

        resource_tracker.flush(cmd_buffer);

        cmd_buffer.beginRendering(renderingInfo);
        subpassInfo.subpass->execute(cmd_buffer, renderArea);
        cmd_buffer.endRendering();
    }
}

RenderingBuilder::RenderingBuilder() = default;

RenderingBuilder &&RenderingBuilder::add_subpass(
    std::shared_ptr<Subpass> subpass,
    std::vector<std::shared_ptr<const ImageView>> color_attachments,
    std::shared_ptr<const ImageView> depth_attachment) && {
    m_subpasses.push_back({std::move(subpass), std::move(color_attachments),
                           std::move(depth_attachment)});
    return std::move(*this);
}

Rendering RenderingBuilder::build() && {
    return Rendering(std::move(m_subpasses));
}

} // namespace vw
