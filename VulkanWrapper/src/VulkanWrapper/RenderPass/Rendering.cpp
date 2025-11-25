#include "VulkanWrapper/RenderPass/Rendering.h"

#include "VulkanWrapper/Memory/Barrier.h"

namespace vw {

Rendering::Rendering(std::vector<SubpassInfo> subpasses)
    : m_subpasses(std::move(subpasses)) {}

void Rendering::execute(vk::CommandBuffer cmd_buffer) const {
    // Track current layout of each image
    std::map<std::shared_ptr<const ImageView>, std::optional<vk::ImageLayout>>
        imageLayouts;

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

                // Get current layout or default to Undefined
                auto &oldLayout = imageLayouts[view];

                execute_image_transition(
                    cmd_buffer, view->image(),
                    oldLayout.value_or(vk::ImageLayout::eUndefined),
                    attachmentInfo.imageLayout);

                // Update tracked layout
                oldLayout = attachmentInfo.imageLayout;

                attachmentInfo.setImageView(view->handle());
                colorIndex++;
            }
        }

        auto renderingInfo = vk::RenderingInfo()
                                 .setRenderArea(renderArea)
                                 .setLayerCount(1)
                                 .setColorAttachments(attachmentInfos);

        auto depthInfo = subpassInfo.subpass->depth_attachment_information();
        if (depthInfo && subpassInfo.depth_attachment) {

            // Get current layout or default to Undefined
            auto &oldLayout = imageLayouts[subpassInfo.depth_attachment];

            execute_image_transition(
                cmd_buffer, subpassInfo.depth_attachment->image(),
                oldLayout.value_or(vk::ImageLayout::eUndefined),
                depthInfo->imageLayout);

            // Update tracked layout
            oldLayout = depthInfo->imageLayout;

            depthInfo->setImageView(subpassInfo.depth_attachment->handle());
            renderingInfo.setPDepthAttachment(&*depthInfo);
        }

        cmd_buffer.beginRendering(renderingInfo);
        subpassInfo.subpass->execute(cmd_buffer);
        cmd_buffer.endRendering();
    }
}

RenderingBuilder::RenderingBuilder() = default;

RenderingBuilder &&RenderingBuilder::add_subpass(
    std::unique_ptr<Subpass> subpass,
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
