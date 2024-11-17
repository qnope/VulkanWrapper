#pragma once

#include "VulkanWrapper/Core/fwd.h"
#include "VulkanWrapper/Core/Vulkan/ObjectWithHandle.h"

namespace vw {
class Pipeline : public ObjectWithUniqueHandle<vk::UniquePipeline> {
  public:
  private:
};

class PipelineBuilder {
  public:
    PipelineBuilder addShaderModule(vk::ShaderStageFlags flags,
                                    ShaderModule module) &&;

  private:
    std::map<vk::ShaderStageFlags, ShaderModule> m_shaderModules;
};
} // namespace vw
