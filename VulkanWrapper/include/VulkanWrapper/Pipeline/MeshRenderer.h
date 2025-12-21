#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"

namespace vw {

/// Renderer for meshes using bindless materials.
/// Descriptor sets for materials are bound globally before rendering.
class MeshRenderer {
  public:
    void add_pipeline(Model::Material::MaterialTypeTag tag,
                      std::shared_ptr<const Pipeline> pipeline);

    /// Draw a mesh. Binds the pipeline for the mesh's material type.
    /// Material descriptor sets should be bound before calling this.
    void draw_mesh(vk::CommandBuffer cmd_buffer, const Model::Mesh &mesh,
                   const glm::mat4 &transform) const;

    [[nodiscard]] std::shared_ptr<const Pipeline>
    pipeline_for(Model::Material::MaterialTypeTag tag) const;

  private:
    std::unordered_map<Model::Material::MaterialTypeTag,
                       std::shared_ptr<const Pipeline>>
        m_pipelines;
};
} // namespace vw
