#include "VulkanWrapper/Pipeline/MeshRenderer.h"

#include "VulkanWrapper/Model/Mesh.h"
#include "VulkanWrapper/Utils/Error.h"

namespace vw {

void MeshRenderer::add_pipeline(Model::Material::MaterialTypeTag tag,
                                std::shared_ptr<const Pipeline> pipeline) {
    m_pipelines.emplace(tag, std::move(pipeline));
}

void MeshRenderer::draw_mesh(vk::CommandBuffer cmd_buffer,
                             const Model::Mesh &mesh,
                             const glm::mat4 &transform) const {
    auto it = m_pipelines.find(mesh.material_type_tag());
    if (it == m_pipelines.end()) {
        throw LogicException::invalid_state(
            "No pipeline registered for material type");
    }
    cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                            it->second->handle());
    mesh.draw(cmd_buffer, it->second->layout(), transform);
}

std::shared_ptr<const Pipeline>
MeshRenderer::pipeline_for(Model::Material::MaterialTypeTag tag) const {
    auto it = m_pipelines.find(tag);
    return it != m_pipelines.end() ? it->second : nullptr;
}

} // namespace vw
