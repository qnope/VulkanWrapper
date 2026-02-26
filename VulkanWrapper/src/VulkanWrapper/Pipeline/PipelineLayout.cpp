module;
#include "VulkanWrapper/3rd_party.h"
module vw;
import "VulkanWrapper/vw_vulkan.h";
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
    std::vector<vk::DescriptorSetLayout> layouts;
    layouts.reserve(m_descriptorSetLayouts.size());
    for (const auto &dsl : m_descriptorSetLayouts) {
        layouts.push_back(dsl->handle());
    }

    const auto info = vk::PipelineLayoutCreateInfo()
                          .setSetLayouts(layouts)
                          .setPushConstantRanges(m_pushConstantRanges);

    auto layout = check_vk(m_device->handle().createPipelineLayoutUnique(info),
                           "Failed to create pipeline layout");

    return {std::move(layout)};
}

} // namespace vw
