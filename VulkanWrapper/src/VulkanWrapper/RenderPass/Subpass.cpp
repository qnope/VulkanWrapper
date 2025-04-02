#include "VulkanWrapper/RenderPass/Subpass.h"

namespace vw {

vk::PipelineBindPoint Subpass::pipeline_bind_point() const noexcept {
    return vk::PipelineBindPoint::eGraphics;
}

const vk::AttachmentReference2 *
Subpass::depth_stencil_attachment() const noexcept {
    return nullptr;
}
} // namespace vw
