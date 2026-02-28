export module vw.model:mesh_manager;
import std3rd;
import vulkan3rd;
import vw.vulkan;
import vw.memory;
import vw.descriptors;
import :material_type_tag;
import :material;
import :mesh;
import :bindless_material_manager;

export namespace vw::Model {

class MeshManager {
  public:
    MeshManager(std::shared_ptr<const Device> device,
                std::shared_ptr<Allocator> allocator);

    void add_mesh(std::vector<FullVertex3D> vertices,
                  std::vector<uint32_t> indices, Material::Material material);

    void read_file(const std::filesystem::path &path);

    [[nodiscard]] vk::CommandBuffer fill_command_buffer();

    [[nodiscard]] const std::vector<Mesh> &meshes() const noexcept;

    [[nodiscard]] Material::BindlessMaterialManager &
    material_manager() noexcept;

    [[nodiscard]] const Material::BindlessMaterialManager &
    material_manager() const noexcept;

  private:
    std::shared_ptr<StagingBufferManager> m_staging_buffer_manager;
    BufferList<Vertex3D, false, VertexBufferUsage> m_vertex_buffer;
    BufferList<FullVertex3D, false, VertexBufferUsage> m_full_vertex_buffer;
    IndexBufferList m_index_buffer;
    Material::BindlessMaterialManager m_material_manager;
    std::vector<Mesh> m_meshes;
};

} // namespace vw::Model
