module vw.model;
import std3rd;
import vulkan3rd;
import vw.utils;
import vw.vulkan;
import vw.memory;
import vw.descriptors;
import vw.pipeline;

namespace vw::Model {

MeshManager::MeshManager(std::shared_ptr<const Device> device,
                         std::shared_ptr<Allocator> allocator)
    : m_staging_buffer_manager{std::make_shared<StagingBufferManager>(
          device, allocator)}
    , m_vertex_buffer{allocator}
    , m_full_vertex_buffer{allocator}
    , m_index_buffer{allocator}
    , m_material_manager{device, allocator, m_staging_buffer_manager} {
    m_material_manager.register_handler<Material::TexturedMaterialHandler>(
        m_material_manager.texture_manager());
    m_material_manager
        .register_handler<Material::EmissiveTexturedMaterialHandler>(
            m_material_manager.texture_manager());
    m_material_manager.register_handler<Material::ColoredMaterialHandler>();
}

void MeshManager::add_mesh(std::vector<FullVertex3D> vertices,
                           std::vector<uint32_t> indices,
                           Material::Material material) {
    auto [full_vertex_buffer, vertex_offset] =
        m_full_vertex_buffer.create_buffer(vertices.size());
    auto [index_buffer, first_index] =
        m_index_buffer.create_buffer(indices.size());
    auto vertex_buffer = m_vertex_buffer.create_buffer(vertices.size()).buffer;

    std::vector<Vertex3D> position_vertices;
    position_vertices.reserve(vertices.size());
    for (const auto &v : vertices) {
        position_vertices.push_back(Vertex3D{v.position});
    }

    m_meshes.emplace_back(vertex_buffer, full_vertex_buffer, index_buffer,
                          material, indices.size(), vertex_offset, first_index,
                          position_vertices.size());

    m_staging_buffer_manager->fill_buffer<Vertex3D>(
        position_vertices, *vertex_buffer, vertex_offset);
    m_staging_buffer_manager->fill_buffer<FullVertex3D>(
        vertices, *full_vertex_buffer, vertex_offset);
    m_staging_buffer_manager->fill_buffer<uint32_t>(indices, *index_buffer,
                                                    first_index);
}

void MeshManager::read_file(const std::filesystem::path &path) {
    import_model(path, *this);
}

vk::CommandBuffer MeshManager::fill_command_buffer() {
    m_material_manager.upload_all();
    return m_staging_buffer_manager->fill_command_buffer();
}

const std::vector<Mesh> &MeshManager::meshes() const noexcept {
    return m_meshes;
}

Material::BindlessMaterialManager &MeshManager::material_manager() noexcept {
    return m_material_manager;
}

const Material::BindlessMaterialManager &
MeshManager::material_manager() const noexcept {
    return m_material_manager;
}

} // namespace vw::Model
