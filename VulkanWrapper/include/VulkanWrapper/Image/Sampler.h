#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

using SamplerCreateException = TaggedException<struct SamplerCreateTag>;

class Sampler : public ObjectWithUniqueHandle<vk::UniqueSampler> {
  public:
    using ObjectWithUniqueHandle<vk::UniqueSampler>::ObjectWithHandle;

  private:
};

class SamplerBuilder {
  public:
    SamplerBuilder(const Device &device);

    std::shared_ptr<const Sampler> build();

  private:
    const Device *m_device;
    vk::SamplerCreateInfo m_info;
};
} // namespace vw
