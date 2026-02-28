export module vw.model:mesh_renderer;
import std3rd;
import vulkan3rd;
import glm3rd;
import :material_type_tag;
import vw.pipeline;

export namespace vw {

namespace Model {
class Mesh;
} // namespace Model

/// Renderer for meshes using bindless materials.
/// Descriptor sets for materials are bound globally before rendering.
class MeshRenderer {
  public:
    void add_pipeline(Model::Material::MaterialTypeTag tag,
                      std::shared_ptr<const Pipeline> pipeline);

    /// Draw a mesh. Binds the pipeline for the mesh's material type.
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
