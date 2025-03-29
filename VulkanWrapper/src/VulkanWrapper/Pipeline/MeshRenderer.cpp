#include "VulkanWrapper/Pipeline/MeshRenderer.h"

#include "VulkanWrapper/Model/Mesh.h"

namespace vw {

void MeshRenderer::add_pipeline(Model::Material::MaterialTypeTag tag,
                                Pipeline pipeline) {
    m_pipelines.emplace(tag, std::move(pipeline));
}

void MeshRenderer::draw_mesh(vk::CommandBuffer cmd_buffer,
                             const Model::Mesh &mesh,
                             vk::DescriptorSet descriptor_set) const {
    auto it = m_pipelines.find(mesh.material_type_tag());
    assert(it != m_pipelines.end());
    cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                            it->second.handle());
    cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                  it->second.layout().handle(), 0,
                                  descriptor_set, nullptr);
    mesh.draw(cmd_buffer, it->second.layout());
}
} // namespace vw
