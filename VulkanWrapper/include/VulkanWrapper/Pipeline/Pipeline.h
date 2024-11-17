#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
using GraphicsPipelineCreationException =
    TaggedException<struct GraphicsPipelineCreationTag>;

class Pipeline : public ObjectWithUniqueHandle<vk::UniquePipeline> {
    friend class GraphicsPipelineBuilder;

  public:
  private:
    using ObjectWithUniqueHandle<vk::UniquePipeline>::ObjectWithUniqueHandle;
};

class GraphicsPipelineBuilder {
  public:
    GraphicsPipelineBuilder addShaderModule(vk::ShaderStageFlagBits flags,
                                            ShaderModule module) &&;

    Pipeline build(Device &device) &&;

  private:
    std::vector<vk::PipelineShaderStageCreateInfo>
    createShaderStageInfos() const noexcept;

  private:
    std::map<vk::ShaderStageFlagBits, ShaderModule> m_shaderModules;
};
} // namespace vw
