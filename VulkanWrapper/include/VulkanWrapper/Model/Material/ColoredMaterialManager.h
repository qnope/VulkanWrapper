#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/BufferList.h"
#include "VulkanWrapper/Model/Material/MaterialManager.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"

namespace vw::Model::Material {
struct ColoredMaterialTag {};
inline const MaterialTypeTag colored_material_tag =
    create_material_type_tag<ColoredMaterialTag>();

template <>
class ConcreteMaterialManager<&colored_material_tag> : public MaterialManager {
  public:
    ConcreteMaterialManager<&colored_material_tag>(
        std::shared_ptr<const Device> device,
        std::shared_ptr<const Allocator> allocator,
        std::shared_ptr<StagingBufferManager> staging_buffer_manager) noexcept;

    Material allocate(const glm::vec4 &color) noexcept;

  private:
    std::shared_ptr<StagingBufferManager> m_staging_buffer_manager;
    BufferList<glm::vec4, false, UniformBufferUsage> m_buffer;
};

using ColoredMaterialManager = ConcreteMaterialManager<&colored_material_tag>;

std::optional<Material>
allocate_colored_material(const Internal::MaterialInfo &info,
                          ColoredMaterialManager &manager);

} // namespace vw::Model::Material
