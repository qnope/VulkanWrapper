#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Model/Material/MaterialManager.h"

namespace vw::Model::Material {
struct TexturedMaterialTag {};
inline const MaterialTypeTag textured_material_tag =
    create_material_type_tag<TexturedMaterialTag>();

template <>
class ConcreteMaterialManager<&textured_material_tag> : public MaterialManager {
  public:
    ConcreteMaterialManager(std::shared_ptr<const Device> device,
                            std::shared_ptr<StagingBufferManager> staging_buffer) noexcept;

    Material allocate(const std::filesystem::path &path);

  private:
    std::shared_ptr<StagingBufferManager> m_staging_buffer;
    std::vector<CombinedImage> m_combined_images;
};

using TextureMaterialManager = ConcreteMaterialManager<&textured_material_tag>;

std::optional<Material>
allocate_textured_material(const Internal::MaterialInfo &info,
                           TextureMaterialManager &manager);

} // namespace vw::Model::Material
