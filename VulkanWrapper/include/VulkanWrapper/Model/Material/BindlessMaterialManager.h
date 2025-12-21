#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/BufferUsage.h"
#include "VulkanWrapper/Model/Material/BindlessTextureManager.h"
#include "VulkanWrapper/Model/Material/Material.h"
#include "VulkanWrapper/Model/Material/MaterialData.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"
#include "VulkanWrapper/fwd.h"

namespace vw::Model::Material {

VW_REGISTER_MATERIAL_TYPE(bindless_textured_material_tag);
VW_REGISTER_MATERIAL_TYPE(bindless_colored_material_tag);

class BindlessMaterialManager {
  public:
    BindlessMaterialManager(std::shared_ptr<const Device> device,
                            std::shared_ptr<Allocator> allocator,
                            std::shared_ptr<StagingBufferManager> staging);

    Material create_textured_material(const std::filesystem::path &path);
    Material create_colored_material(const glm::vec4 &color);

    [[nodiscard]] vk::Buffer textured_materials_ssbo() const noexcept;
    [[nodiscard]] vk::Buffer colored_materials_ssbo() const noexcept;

    [[nodiscard]] BindlessTextureManager &texture_manager() noexcept;
    [[nodiscard]] const BindlessTextureManager &texture_manager() const noexcept;

    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    textured_layout() const noexcept;
    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    colored_layout() const noexcept;

    [[nodiscard]] vk::DescriptorSet textured_descriptor_set() const noexcept;
    [[nodiscard]] vk::DescriptorSet colored_descriptor_set() const noexcept;

    void upload_materials();

    [[nodiscard]] std::vector<Barrier::ResourceState> get_resources() const;

  private:
    using TexturedSSBO = Buffer<TexturedMaterialData, false, StorageBufferUsage>;
    using ColoredSSBO = Buffer<ColoredMaterialData, false, StorageBufferUsage>;

    void create_textured_layout();
    void create_colored_layout();
    void create_textured_descriptor_pool();
    void create_colored_descriptor_pool();
    void allocate_descriptor_sets();
    void update_textured_descriptors();
    void update_colored_descriptors();

    std::shared_ptr<const Device> m_device;
    std::shared_ptr<Allocator> m_allocator;
    std::shared_ptr<StagingBufferManager> m_staging;

    BindlessTextureManager m_texture_manager;

    std::vector<TexturedMaterialData> m_textured_data;
    std::vector<ColoredMaterialData> m_colored_data;

    std::optional<TexturedSSBO> m_textured_ssbo;
    std::optional<ColoredSSBO> m_colored_ssbo;

    std::shared_ptr<DescriptorSetLayout> m_textured_layout;
    std::shared_ptr<DescriptorSetLayout> m_colored_layout;

    vk::UniqueDescriptorPool m_textured_pool;
    vk::UniqueDescriptorPool m_colored_pool;

    vk::DescriptorSet m_textured_descriptor_set;
    vk::DescriptorSet m_colored_descriptor_set;

    bool m_dirty = false;
};

} // namespace vw::Model::Material
