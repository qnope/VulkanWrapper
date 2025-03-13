#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"

#include "VulkanWrapper/Vulkan/Device.h"
#include <source_location>

namespace vw {
DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder(const Device &device)
    : m_device{device} {}

DescriptorSetLayoutBuilder &&
DescriptorSetLayoutBuilder::with_uniform_buffer(vk::ShaderStageFlags stages,
                                                int number) {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    auto binding = vk::DescriptorSetLayoutBinding()
                       .setBinding(m_current_binding)
                       .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                       .setStageFlags(stages)
                       .setDescriptorCount(number);
    m_current_binding += number;
    bindings.push_back(binding);
    return std::move(*this);
}

std::shared_ptr<DescriptorSetLayout> DescriptorSetLayoutBuilder::build() && {
    const auto info =
        vk::DescriptorSetLayoutCreateInfo().setBindings(m_bindings);
    auto [result, value] =
        m_device.handle().createDescriptorSetLayoutUnique(info);
    if (result != vk::Result::eSuccess)
        throw DescriptorSetLayoutCreationException{
            std::source_location::current()};
    return std::make_shared<DescriptorSetLayout>(std::move(value));
}

} // namespace vw
