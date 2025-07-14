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
    DescriptorSetLayout(std::vector<vk::DescriptorSetLayoutBinding> pool_sizes,
                        vk::UniqueDescriptorSetLayout set_layout);

    [[nodiscard]] std::vector<vk::DescriptorPoolSize> get_pool_sizes() const;

  private:
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
};

class DescriptorSetLayoutBuilder {
  public:
    DescriptorSetLayoutBuilder(const Device &device);

    DescriptorSetLayoutBuilder &&
    with_uniform_buffer(vk::ShaderStageFlags stages, int number) &&;

    DescriptorSetLayoutBuilder &&with_sampled_image(vk::ShaderStageFlags stages,
                                                    int number) &&;

    DescriptorSetLayoutBuilder &&
    with_combined_image(vk::ShaderStageFlags stages, int number) &&;

    DescriptorSetLayoutBuilder &&
    with_input_attachment(vk::ShaderStageFlags stages) &&;

    DescriptorSetLayoutBuilder &&
    with_acceleration_structure(vk::ShaderStageFlags stages) &&;

    DescriptorSetLayoutBuilder &&with_storage_image(vk::ShaderStageFlags stages,
                                                    int number) &&;

    std::shared_ptr<DescriptorSetLayout> build() &&;

  private:
    const Device *m_device;
    int m_current_binding = 0;
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
};

} // namespace vw
