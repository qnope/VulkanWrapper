#pragma once
#include "VulkanWrapper/Model/Material/MaterialData.h"
#include "VulkanWrapper/Model/Material/MaterialTypeHandler.h"

struct aiMaterial;

namespace vw::Model::Material {

class BindlessTextureManager;

VW_DECLARE_MATERIAL_TYPE(emissive_textured_material_tag);

class EmissiveTexturedMaterialHandler
    : public MaterialTypeHandler<EmissiveTexturedMaterialData> {
    friend class MaterialTypeHandler<EmissiveTexturedMaterialData>;

  public:
    using Base = MaterialTypeHandler<EmissiveTexturedMaterialData>;

    [[nodiscard]] MaterialTypeTag tag() const override;
    [[nodiscard]] MaterialPriority priority() const override;

    [[nodiscard]] std::optional<vk::DescriptorSet>
    additional_descriptor_set() const override;

    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    additional_descriptor_set_layout() const override;

    [[nodiscard]] Material
    create_material(const std::filesystem::path &texture_path, float intensity);

  protected:
    [[nodiscard]] std::optional<EmissiveTexturedMaterialData>
    try_create_gpu_data(const aiMaterial *mat,
                        const std::filesystem::path &base_path) override;

    [[nodiscard]] std::vector<Barrier::ResourceState>
    get_texture_resources() const override;

  private:
    EmissiveTexturedMaterialHandler(std::shared_ptr<const Device> device,
                                    std::shared_ptr<Allocator> allocator,
                                    BindlessTextureManager &texture_manager);

    BindlessTextureManager &m_texture_manager;
};

} // namespace vw::Model::Material
