#include "RenderPass/Subpass.h"

#include <VulkanWrapper/RenderPass/Attachment.h>
#include <VulkanWrapper/RenderPass/Subpass.h>

vw::Subpass *vw_create_subpass(const vw_AttachmentSubpass *attachments,
                               int size) {

    vw::SubpassBuilder builder;
    for (int i = 0; i < size; ++i) {
        auto [cAttachment, currentLayout] = attachments[i];
        auto attachment =
            vw::AttachmentBuilder(cAttachment.id)
                .with_final_layout(vk::ImageLayout(cAttachment.finalLayout))
                .with_format(vk::Format(cAttachment.format))
                .build();

        auto newBuilder = std::move(builder).add_color_attachment(
            std::move(attachment), vk::ImageLayout(currentLayout));

        builder = std::move(newBuilder);
    }
    return new vw::Subpass{std::move(builder).build()};
}

void vw_destroy_subpass(vw::Subpass *subpass) { delete subpass; }
