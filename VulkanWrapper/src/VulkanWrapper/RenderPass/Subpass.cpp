#include "VulkanWrapper/RenderPass/Subpass.h"

namespace vw {

vk::PipelineBindPoint ISubpass::pipeline_bind_point() const noexcept {
    return vk::PipelineBindPoint::eGraphics;
}

} // namespace vw
