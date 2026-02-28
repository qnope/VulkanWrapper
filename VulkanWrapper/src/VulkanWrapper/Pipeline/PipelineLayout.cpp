module vw.pipeline;
import std3rd;
import vulkan3rd;
import vw.utils;
import vw.vulkan;
import vw.descriptors;

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
    // Rewrite to_handle pipe as explicit loop to avoid ADL issues in module
    // implementation units
    std::vector<vk::DescriptorSetLayout> layouts;
    layouts.reserve(m_descriptorSetLayouts.size());
    for (const auto &layout : m_descriptorSetLayouts) {
        layouts.push_back(layout->handle());
    }

    const auto info = vk::PipelineLayoutCreateInfo()
                          .setSetLayouts(layouts)
                          .setPushConstantRanges(m_pushConstantRanges);

    auto layout = check_vk(m_device->handle().createPipelineLayoutUnique(info),
                           "Failed to create pipeline layout");

    return {std::move(layout)};
}

} // namespace vw
