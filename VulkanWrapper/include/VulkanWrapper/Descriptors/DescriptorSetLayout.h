#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
using DescriptorSetLayoutCreationException =
    TaggedException<struct DescriptorSetLayoutCreationTag>;

class DescriptorSetLayout
    : public ObjectWithUniqueHandle<vk::UniqueDescriptorSetLayout> {
  public:
  private:
};

class DescriptorSetLayoutBuilder {
  public:
    DescriptorSetLayoutBuilder(const Device &device);

    DescriptorSetLayoutBuilder &&
    with_uniform_buffer(vk::ShaderStageFlags stages, int number);

    std::shared_ptr<DescriptorSetLayout> build() &&;

  private:
    const Device &m_device;
    int m_current_binding = 0;
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
};

} // namespace vw
