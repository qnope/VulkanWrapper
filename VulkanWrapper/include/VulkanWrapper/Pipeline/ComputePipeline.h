#pragma once
#include "VulkanWrapper/Pipeline/Pipeline.h"

namespace vw {

class ComputePipelineBuilder {
  public:
    ComputePipelineBuilder(std::shared_ptr<const Device> device,
                           PipelineLayout pipelineLayout);

    ComputePipelineBuilder &
    set_shader(std::shared_ptr<const ShaderModule> module);

    std::shared_ptr<const Pipeline> build();

  private:
    std::shared_ptr<const Device> m_device;
    PipelineLayout m_pipelineLayout;
    std::shared_ptr<const ShaderModule> m_shaderModule;
};
} // namespace vw
