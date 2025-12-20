#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"

namespace vw {
class MeshRenderer {
  public:
    void add_pipeline(Model::Material::MaterialTypeTag tag,
                      std::shared_ptr<const Pipeline> pipeline);

    void draw_mesh(vk::CommandBuffer cmd_buffer, const Model::Mesh &mesh,
                   std::span<const vk::DescriptorSet> first_descriptor_sets,
                   const glm::mat4 &transform) const;

  private:
    std::unordered_map<Model::Material::MaterialTypeTag,
                       std::shared_ptr<const Pipeline>>
        m_pipelines;
};
} // namespace vw
