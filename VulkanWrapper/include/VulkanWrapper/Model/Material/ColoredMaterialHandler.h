#pragma once
#include "VulkanWrapper/Model/Material/MaterialData.h"
#include "VulkanWrapper/Model/Material/MaterialTypeHandler.h"

struct aiMaterial;

namespace vw::Model::Material {

VW_DECLARE_MATERIAL_TYPE(colored_material_tag);

class ColoredMaterialHandler : public MaterialTypeHandler<ColoredMaterialData> {
    friend class MaterialTypeHandler<ColoredMaterialData>;

  public:
    using Base = MaterialTypeHandler<ColoredMaterialData>;

    [[nodiscard]] MaterialTypeTag tag() const override;
    [[nodiscard]] MaterialPriority priority() const override;

  protected:
    [[nodiscard]] std::optional<ColoredMaterialData>
    try_create_gpu_data(const aiMaterial *mat,
                        const std::filesystem::path &base_path) override;

    void setup_layout(DescriptorSetLayoutBuilder &builder) override;

    void setup_descriptors(DescriptorAllocator &alloc) override;

  private:
    ColoredMaterialHandler(std::shared_ptr<const Device> device,
                           std::shared_ptr<Allocator> allocator,
                           BindlessTextureManager &texture_manager);
};

} // namespace vw::Model::Material
