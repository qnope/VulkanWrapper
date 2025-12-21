#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include <filesystem>
#include <unordered_map>

namespace vw::Model::Material {

class BindlessTextureManager {
  public:
    static constexpr uint32_t MAX_TEXTURES = 4096;

    BindlessTextureManager(std::shared_ptr<const Device> device,
                           std::shared_ptr<Allocator> allocator,
                           std::shared_ptr<StagingBufferManager> staging);

    uint32_t register_texture(const std::filesystem::path &path);

    [[nodiscard]] vk::DescriptorSet descriptor_set() const noexcept;

    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    layout() const noexcept;

    [[nodiscard]] uint32_t texture_count() const noexcept;

    void update_descriptors();

    [[nodiscard]] std::vector<Barrier::ResourceState> get_resources() const;

    [[nodiscard]] vk::Sampler sampler() const noexcept;

    // Write texture image descriptors to external descriptor set
    void write_image_descriptors(vk::DescriptorSet dest_set,
                                 uint32_t dest_binding) const;

  private:
    void create_descriptor_pool();
    void allocate_descriptor_set();

    std::shared_ptr<const Device> m_device;
    std::shared_ptr<Allocator> m_allocator;
    std::shared_ptr<StagingBufferManager> m_staging;

    std::shared_ptr<DescriptorSetLayout> m_layout;
    vk::UniqueDescriptorPool m_pool;
    vk::DescriptorSet m_descriptor_set;
    std::shared_ptr<const Sampler> m_sampler;

    std::vector<CombinedImage> m_combined_images;
    std::unordered_map<std::filesystem::path, uint32_t> m_path_to_index;

    uint32_t m_last_updated_count = 0;
};

} // namespace vw::Model::Material
