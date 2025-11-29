#include "VulkanWrapper/RenderPass/Rendering.h"

#include "VulkanWrapper/Memory/Barrier.h"

namespace vw {

Rendering::Rendering(std::vector<std::shared_ptr<Subpass>> subpasses)
    : m_subpasses(std::move(subpasses)) {}

void Rendering::execute(vk::CommandBuffer cmd_buffer,
                        Barrier::ResourceTracker &resource_tracker,
                        int image_index) const {
    for (const auto &subpass : m_subpasses) {
        auto [color, depth, renderArea] =
            subpass->attachment_information(image_index);

        auto renderingInfo = vk::RenderingInfo()
                                 .setRenderArea(renderArea)
                                 .setLayerCount(1)
                                 .setColorAttachments(color);

        if (depth) {
            renderingInfo.setPDepthAttachment(&*depth);
        }

        for (const auto &resource : subpass->resource_states(image_index)) {
            resource_tracker.request(resource);
        }

        resource_tracker.flush(cmd_buffer);

        cmd_buffer.beginRendering(renderingInfo);
        subpass->execute(cmd_buffer, image_index);
        cmd_buffer.endRendering();
    }
}

RenderingBuilder::RenderingBuilder() = default;

RenderingBuilder &&
RenderingBuilder::add_subpass(std::shared_ptr<Subpass> subpass) && {
    m_subpasses.push_back(std::move(subpass));
    return std::move(*this);
}

Rendering RenderingBuilder::build() && {
    return Rendering(std::move(m_subpasses));
}

} // namespace vw
