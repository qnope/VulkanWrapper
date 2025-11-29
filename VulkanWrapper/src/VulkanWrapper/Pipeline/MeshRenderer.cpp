#include "VulkanWrapper/Pipeline/MeshRenderer.h"

#include "VulkanWrapper/Model/Mesh.h"

namespace vw {

void MeshRenderer::add_pipeline(Model::Material::MaterialTypeTag tag,
                                std::shared_ptr<const Pipeline> pipeline) {
    m_pipelines.emplace(tag, std::move(pipeline));
}

void MeshRenderer::draw_mesh(
    vk::CommandBuffer cmd_buffer, const Model::Mesh &mesh,
    std::span<const vk::DescriptorSet> first_descriptor_sets) const {
    auto it = m_pipelines.find(mesh.material_type_tag());
    assert(it != m_pipelines.end());
    cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                            it->second->handle());
    cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                  it->second->layout().handle(), 0,
                                  first_descriptor_sets, nullptr);
    mesh.draw(cmd_buffer, it->second->layout(), first_descriptor_sets.size());
}
} // namespace vw
