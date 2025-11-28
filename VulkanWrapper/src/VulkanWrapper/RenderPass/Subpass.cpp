#include "VulkanWrapper/RenderPass/Subpass.h"
#include <source_location>

namespace vw {

vk::PipelineBindPoint Subpass::pipeline_bind_point() const noexcept {
    return vk::PipelineBindPoint::eGraphics;
}





} // namespace vw
