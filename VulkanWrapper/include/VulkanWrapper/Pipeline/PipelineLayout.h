#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
using PipelineLayoutCreationException =
    TaggedException<struct PipelineLayoutCreationTag>;

class PipelineLayout : public ObjectWithUniqueHandle<vk::UniquePipelineLayout> {
    friend class PipelineLayoutBuilder;

  public:
  private:
    using ObjectWithUniqueHandle<
        vk::UniquePipelineLayout>::ObjectWithUniqueHandle;
};

class PipelineLayoutBuilder {
  public:
    PipelineLayout build(Device &device) &&;

  private:
};
} // namespace vw
