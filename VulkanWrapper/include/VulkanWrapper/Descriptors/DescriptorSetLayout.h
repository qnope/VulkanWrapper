#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

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
    DescriptorSetLayoutBuilder(std::shared_ptr<const Device> device);

    DescriptorSetLayoutBuilder &with_uniform_buffer(vk::ShaderStageFlags stages,
                                                    int number);

    DescriptorSetLayoutBuilder &with_sampled_image(vk::ShaderStageFlags stages,
                                                   int number);

    DescriptorSetLayoutBuilder &with_combined_image(vk::ShaderStageFlags stages,
                                                    int number);

    DescriptorSetLayoutBuilder &
    with_input_attachment(vk::ShaderStageFlags stages);

    DescriptorSetLayoutBuilder &
    with_acceleration_structure(vk::ShaderStageFlags stages);

    DescriptorSetLayoutBuilder &with_storage_image(vk::ShaderStageFlags stages,
                                                   int number);

    DescriptorSetLayoutBuilder &with_storage_buffer(vk::ShaderStageFlags stages,
                                                    int number = 1);

    DescriptorSetLayoutBuilder &with_sampler(vk::ShaderStageFlags stages);

    DescriptorSetLayoutBuilder &
    with_sampled_images_bindless(vk::ShaderStageFlags stages,
                                 uint32_t max_count);

    DescriptorSetLayoutBuilder &
    with_storage_buffer_bindless(vk::ShaderStageFlags stages);

    std::shared_ptr<DescriptorSetLayout> build();

  private:
    std::shared_ptr<const Device> m_device;
    uint32_t m_current_binding = 0;
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
    std::vector<vk::DescriptorBindingFlags> m_binding_flags;
    bool m_has_bindless = false;
};

} // namespace vw
