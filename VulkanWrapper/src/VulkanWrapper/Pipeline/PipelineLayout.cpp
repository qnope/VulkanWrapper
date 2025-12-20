#include "VulkanWrapper/Pipeline/PipelineLayout.h"

#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
PipelineLayoutBuilder::PipelineLayoutBuilder(
    std::shared_ptr<const Device> device)
    : m_device{std::move(device)} {}

PipelineLayoutBuilder &PipelineLayoutBuilder::with_descriptor_set_layout(
    std::shared_ptr<const DescriptorSetLayout> layout) {

    m_descriptorSetLayouts.push_back(std::move(layout));
    return *this;
}

PipelineLayoutBuilder &
PipelineLayoutBuilder::with_push_constant_range(vk::PushConstantRange range) {
    m_pushConstantRanges.push_back(range);
    return *this;
}

PipelineLayout PipelineLayoutBuilder::build() {
    auto layouts = m_descriptorSetLayouts | to_handle | to<std::vector>;

    const auto info = vk::PipelineLayoutCreateInfo()
                          .setSetLayouts(layouts)
                          .setPushConstantRanges(m_pushConstantRanges);

    auto layout = check_vk(m_device->handle().createPipelineLayoutUnique(info),
                           "Failed to create pipeline layout");

    return {std::move(layout)};
}

} // namespace vw
