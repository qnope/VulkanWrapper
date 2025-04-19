#include "VulkanWrapper/RenderPass/Subpass.h"

namespace vw {

static std::vector<vk::AttachmentReference2> empty_attachments;

vk::PipelineBindPoint Subpass::pipeline_bind_point() const noexcept {
    return vk::PipelineBindPoint::eGraphics;
}

const vk::AttachmentReference2 *
Subpass::depth_stencil_attachment() const noexcept {
    return nullptr;
}

const std::vector<vk::AttachmentReference2> &
Subpass::color_attachments() const noexcept {
    return empty_attachments;
}

const std::vector<vk::AttachmentReference2> &
Subpass::input_attachments() const noexcept {
    return empty_attachments;
}

} // namespace vw
