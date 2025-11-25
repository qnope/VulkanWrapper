#include "VulkanWrapper/RenderPass/Subpass.h"
#include <source_location>

namespace vw {

vk::PipelineBindPoint Subpass::pipeline_bind_point() const noexcept {
    return vk::PipelineBindPoint::eGraphics;
}

vk::RenderingAttachmentInfo Subpass::depth_attachment_information() const {
    throw SubpassNotManagingDepthException{std::source_location::current()};
}

} // namespace vw
