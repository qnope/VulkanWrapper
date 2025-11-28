#include "VulkanWrapper/RenderPass/Rendering.h"

#include "VulkanWrapper/Memory/Barrier.h"

namespace vw {

Rendering::Rendering(std::vector<SubpassInfo> subpasses)
    : m_subpasses(std::move(subpasses)) {}

void Rendering::execute(vk::CommandBuffer cmd_buffer,
                        Barrier::ResourceTracker &resource_tracker) const {
    for (const auto &[subpass, color_attachments, depth_attachment] :
         m_subpasses) {
        auto [color, depth] = subpass->attachment_information(
            color_attachments, depth_attachment);

        vk::Rect2D renderArea;
        if (!color_attachments.empty()) {
            renderArea.extent = color_attachments.front()->image()->extent2D();
        } else if (depth_attachment) {
            renderArea.extent = depth_attachment->image()->extent2D();
        }

        auto renderingInfo = vk::RenderingInfo()
                                 .setRenderArea(renderArea)
                                 .setLayerCount(1)
                                 .setColorAttachments(color);

        if (depth) {
            renderingInfo.setPDepthAttachment(&*depth);
        }

        for (const auto &resource :
             subpass->resource_states(color_attachments, depth_attachment)) {
            resource_tracker.request(resource);
        }

        resource_tracker.flush(cmd_buffer);

        cmd_buffer.beginRendering(renderingInfo);
        subpass->execute(cmd_buffer, renderArea);
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
