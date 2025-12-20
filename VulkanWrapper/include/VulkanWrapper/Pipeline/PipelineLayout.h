#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

class PipelineLayout : public ObjectWithUniqueHandle<vk::UniquePipelineLayout> {
    friend class PipelineLayoutBuilder;

  public:
  private:
    using ObjectWithUniqueHandle<
        vk::UniquePipelineLayout>::ObjectWithUniqueHandle;
};

class PipelineLayoutBuilder {
  public:
    PipelineLayoutBuilder(std::shared_ptr<const Device> device);

    PipelineLayoutBuilder &with_descriptor_set_layout(
        std::shared_ptr<const DescriptorSetLayout> layout);

    PipelineLayoutBuilder &
    with_push_constant_range(vk::PushConstantRange range);

    PipelineLayout build();

  private:
    [[nodiscard]] vk::UniqueDescriptorSetLayout build_set_layout(
        const std::vector<vk::DescriptorSetLayoutBinding> &bindings) const;

    std::shared_ptr<const Device> m_device;
    std::vector<std::shared_ptr<const DescriptorSetLayout>>
        m_descriptorSetLayouts;
    std::vector<vk::PushConstantRange> m_pushConstantRanges;
};
} // namespace vw
