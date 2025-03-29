#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"

namespace vw {
class MeshRenderer {
  public:
    void add_pipeline(Model::Material::MaterialTypeTag tag, Pipeline pipeline);
    void draw_mesh(vk::CommandBuffer cmd_buffer, const Model::Mesh &mesh,
                   vk::DescriptorSet descriptor_set) const;

  private:
    std::unordered_map<Model::Material::MaterialTypeTag, Pipeline> m_pipelines;
};
} // namespace vw
