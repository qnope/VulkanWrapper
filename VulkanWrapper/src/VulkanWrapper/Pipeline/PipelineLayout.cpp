#include "VulkanWrapper/Pipeline/PipelineLayout.h"

#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
PipelineLayoutBuilder::PipelineLayoutBuilder(const Device &device)
    : m_device{&device} {}

PipelineLayoutBuilder &&PipelineLayoutBuilder::with_descriptor_set_layout(
    std::shared_ptr<const DescriptorSetLayout> layout) && {

    m_descriptorSetLayouts.push_back(std::move(layout));
    return std::move(*this);
}

PipelineLayout PipelineLayoutBuilder::build() && {
    auto layouts = m_descriptorSetLayouts | to_handle | to<std::vector>;

    const auto info = vk::PipelineLayoutCreateInfo().setSetLayouts(layouts);

    auto [result, layout] = m_device->handle().createPipelineLayoutUnique(info);

    if (result != vk::Result::eSuccess) {
        throw PipelineLayoutCreationException{std::source_location::current()};
    }

    return {std::move(layout)};
}

} // namespace vw
