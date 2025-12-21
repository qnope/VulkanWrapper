#pragma once
#include "VulkanWrapper/Model/Material/MaterialData.h"
#include "VulkanWrapper/Model/Material/MaterialTypeHandler.h"

struct aiMaterial;

namespace vw::Model::Material {

VW_DECLARE_MATERIAL_TYPE(textured_material_tag);

class TexturedMaterialHandler
    : public MaterialTypeHandler<TexturedMaterialData> {
    friend class MaterialTypeHandler<TexturedMaterialData>;

  public:
    using Base = MaterialTypeHandler<TexturedMaterialData>;

    [[nodiscard]] MaterialTypeTag tag() const override;
    [[nodiscard]] MaterialPriority priority() const override;

  protected:
    [[nodiscard]] std::optional<TexturedMaterialData>
    try_create_gpu_data(const aiMaterial *mat,
                        const std::filesystem::path &base_path) override;

    void setup_layout(DescriptorSetLayoutBuilder &builder) override;

    void setup_descriptors(DescriptorAllocator &alloc) override;

    [[nodiscard]] std::vector<Barrier::ResourceState>
    get_texture_resources() const override;

  private:
    TexturedMaterialHandler(std::shared_ptr<const Device> device,
                            std::shared_ptr<Allocator> allocator,
                            BindlessTextureManager &texture_manager);
};

} // namespace vw::Model::Material
