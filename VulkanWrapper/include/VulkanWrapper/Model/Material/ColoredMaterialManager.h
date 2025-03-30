#pragma once

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
        const Device &device, const Allocator &allocator,
        StagingBufferManager &staging_buffer_manager) noexcept;

    Material allocate(const glm::vec4 &color) noexcept;

  private:
    StagingBufferManager *m_staging_buffer_manager;
    BufferList<glm::vec4, false, UniformBufferUsage> m_buffer;
};

using ColoredMaterialManager = ConcreteMaterialManager<&colored_material_tag>;

} // namespace vw::Model::Material
