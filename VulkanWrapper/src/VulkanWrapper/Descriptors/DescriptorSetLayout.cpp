#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"

#include "VulkanWrapper/Vulkan/Device.h"
#include <source_location>

namespace vw {

DescriptorSetLayout::DescriptorSetLayout(
    std::vector<vk::DescriptorSetLayoutBinding> pool_sizes,
    vk::UniqueDescriptorSetLayout set_layout)
    : ObjectWithUniqueHandle<vk::UniqueDescriptorSetLayout>{std::move(
          set_layout)}
    , m_bindings{std::move(pool_sizes)} {}

std::vector<vk::DescriptorPoolSize>
DescriptorSetLayout::get_pool_sizes() const {
    std::vector<vk::DescriptorPoolSize> pool_sizes;
    std::map<vk::DescriptorType, uint32_t> size;

    for (const auto &binding : m_bindings) {
        size[binding.descriptorType] += binding.descriptorCount;
    }

    pool_sizes.reserve(size.size());
    for (auto [type, number] : size) {
        pool_sizes.emplace_back(type, number);
    }

    return pool_sizes;
}

DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder(const Device &device)
    : m_device{&device} {}

DescriptorSetLayoutBuilder &&
DescriptorSetLayoutBuilder::with_uniform_buffer(vk::ShaderStageFlags stages,
                                                int number) && {
    auto binding = vk::DescriptorSetLayoutBinding()
                       .setBinding(m_current_binding)
                       .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                       .setStageFlags(stages)
                       .setDescriptorCount(number);
    m_current_binding += number;
    m_bindings.push_back(binding);
    return std::move(*this);
}

DescriptorSetLayoutBuilder &&
DescriptorSetLayoutBuilder::with_sampled_image(vk::ShaderStageFlags stages,
                                               int number) && {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eSampledImage)
            .setStageFlags(stages)
            .setDescriptorCount(number);
    m_current_binding += number;
    m_bindings.push_back(binding);
    return std::move(*this);
}

DescriptorSetLayoutBuilder &&
DescriptorSetLayoutBuilder::with_combined_image(vk::ShaderStageFlags stages,
                                                int number) && {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(stages)
            .setDescriptorCount(number);
    m_current_binding += number;
    m_bindings.push_back(binding);
    return std::move(*this);
}

DescriptorSetLayoutBuilder &&
DescriptorSetLayoutBuilder::with_combined_image_sampler(vk::ShaderStageFlags stages,
                                                        int number) && {
    return std::move(*this).with_combined_image(stages, number);
}

DescriptorSetLayoutBuilder &&DescriptorSetLayoutBuilder::with_input_attachment(
    vk::ShaderStageFlags stages) && {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eInputAttachment)
            .setStageFlags(stages)
            .setDescriptorCount(1);
    m_current_binding += 1;
    m_bindings.push_back(binding);
    return std::move(*this);
}

DescriptorSetLayoutBuilder &&
DescriptorSetLayoutBuilder::with_acceleration_structure(vk::ShaderStageFlags stages) && {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
            .setStageFlags(stages)
            .setDescriptorCount(1);
    m_current_binding += 1;
    m_bindings.push_back(binding);
    return std::move(*this);
}

DescriptorSetLayoutBuilder &&
DescriptorSetLayoutBuilder::with_storage_image(vk::ShaderStageFlags stages,
                                               int number) && {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eStorageImage)
            .setStageFlags(stages)
            .setDescriptorCount(number);
    m_current_binding += number;
    m_bindings.push_back(binding);
    return std::move(*this);
}

std::shared_ptr<DescriptorSetLayout> DescriptorSetLayoutBuilder::build() && {
    const auto info =
        vk::DescriptorSetLayoutCreateInfo().setBindings(m_bindings);
    auto [result, value] =
        m_device->handle().createDescriptorSetLayoutUnique(info);
    if (result != vk::Result::eSuccess) {
        throw DescriptorSetLayoutCreationException{
            std::source_location::current()};
    }
    return std::make_shared<DescriptorSetLayout>(std::move(m_bindings),
                                                 std::move(value));
}

} // namespace vw
