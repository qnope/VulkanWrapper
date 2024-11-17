#include "VulkanWrapper/Pipeline/Pipeline.h"

#include "VulkanWrapper/Pipeline/ShaderModule.h"

namespace vw {
PipelineBuilder PipelineBuilder::addShaderModule(vk::ShaderStageFlags flags,
                                                 ShaderModule module) && {

    m_shaderModules.emplace(flags, std::move(module));
    return std::move(*this);
}
} // namespace vw
