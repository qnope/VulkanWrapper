#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"

#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

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

DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder(
    std::shared_ptr<const Device> device)
    : m_device{std::move(device)} {}

DescriptorSetLayoutBuilder &
DescriptorSetLayoutBuilder::with_uniform_buffer(vk::ShaderStageFlags stages,
                                                int number) {
    auto binding = vk::DescriptorSetLayoutBinding()
                       .setBinding(m_current_binding)
                       .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                       .setStageFlags(stages)
                       .setDescriptorCount(number);
    m_current_binding += number;
    m_bindings.push_back(binding);
    m_binding_flags.push_back({});
    return *this;
}

DescriptorSetLayoutBuilder &
DescriptorSetLayoutBuilder::with_sampled_image(vk::ShaderStageFlags stages,
                                               int number) {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eSampledImage)
            .setStageFlags(stages)
            .setDescriptorCount(number);
    m_current_binding += number;
    m_bindings.push_back(binding);
    m_binding_flags.push_back({});
    return *this;
}

DescriptorSetLayoutBuilder &
DescriptorSetLayoutBuilder::with_combined_image(vk::ShaderStageFlags stages,
                                                int number) {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(stages)
            .setDescriptorCount(number);
    m_current_binding += number;
    m_bindings.push_back(binding);
    m_binding_flags.push_back({});
    return *this;
}

DescriptorSetLayoutBuilder &
DescriptorSetLayoutBuilder::with_input_attachment(vk::ShaderStageFlags stages) {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eInputAttachment)
            .setStageFlags(stages)
            .setDescriptorCount(1);
    m_current_binding += 1;
    m_bindings.push_back(binding);
    m_binding_flags.push_back({});
    return *this;
}

DescriptorSetLayoutBuilder &
DescriptorSetLayoutBuilder::with_acceleration_structure(
    vk::ShaderStageFlags stages) {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
            .setStageFlags(stages)
            .setDescriptorCount(1);
    m_current_binding += 1;
    m_bindings.push_back(binding);
    m_binding_flags.push_back({});
    return *this;
}

DescriptorSetLayoutBuilder &
DescriptorSetLayoutBuilder::with_storage_image(vk::ShaderStageFlags stages,
                                               int number) {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eStorageImage)
            .setStageFlags(stages)
            .setDescriptorCount(number);
    m_current_binding += number;
    m_bindings.push_back(binding);
    m_binding_flags.push_back({});
    return *this;
}

DescriptorSetLayoutBuilder &
DescriptorSetLayoutBuilder::with_storage_buffer(vk::ShaderStageFlags stages,
                                                int number) {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setStageFlags(stages)
            .setDescriptorCount(number);
    m_current_binding += number;
    m_bindings.push_back(binding);
    m_binding_flags.push_back({});
    return *this;
}

DescriptorSetLayoutBuilder &
DescriptorSetLayoutBuilder::with_sampler(vk::ShaderStageFlags stages) {
    const auto binding = vk::DescriptorSetLayoutBinding()
                             .setBinding(m_current_binding)
                             .setDescriptorType(vk::DescriptorType::eSampler)
                             .setStageFlags(stages)
                             .setDescriptorCount(1);
    m_bindings.push_back(binding);
    m_binding_flags.push_back({});
    ++m_current_binding;
    return *this;
}

DescriptorSetLayoutBuilder &
DescriptorSetLayoutBuilder::with_sampled_images_bindless(
    vk::ShaderStageFlags stages, uint32_t max_count) {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eSampledImage)
            .setStageFlags(stages)
            .setDescriptorCount(max_count);
    m_bindings.push_back(binding);
    m_binding_flags.push_back(vk::DescriptorBindingFlagBits::ePartiallyBound |
                              vk::DescriptorBindingFlagBits::eUpdateAfterBind);
    m_has_bindless = true;
    ++m_current_binding;
    return *this;
}

DescriptorSetLayoutBuilder &
DescriptorSetLayoutBuilder::with_storage_buffer_bindless(
    vk::ShaderStageFlags stages) {
    const auto binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(m_current_binding)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setStageFlags(stages)
            .setDescriptorCount(1);
    m_bindings.push_back(binding);
    m_binding_flags.push_back({});
    ++m_current_binding;
    return *this;
}

std::shared_ptr<DescriptorSetLayout> DescriptorSetLayoutBuilder::build() {
    vk::DescriptorSetLayoutCreateInfo info;
    info.setBindings(m_bindings);

    vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info;
    if (m_has_bindless) {
        binding_flags_info.setBindingFlags(m_binding_flags);
        info.setFlags(
                vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
            .setPNext(&binding_flags_info);
    }

    auto value =
        check_vk(m_device->handle().createDescriptorSetLayoutUnique(info),
                 "Failed to create descriptor set layout");
    return std::make_shared<DescriptorSetLayout>(std::move(m_bindings),
                                                 std::move(value));
}

} // namespace vw
