#include "VulkanWrapper/Model/MeshManager.h"

#include "VulkanWrapper/Model/Importer.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialHandler.h"
#include "VulkanWrapper/Model/Material/TexturedMaterialHandler.h"

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
    m_material_manager.register_handler<Material::ColoredMaterialHandler>();
}

void MeshManager::read_file(const std::filesystem::path &path) {
    import_model(path, *this);
}

const Mesh &MeshManager::add_mesh(std::span<const FullVertex3D> vertices,
                                  std::span<const uint32_t> indices,
                                  Material::Material material) {
    auto simple_vertices = vertices |
                           std::views::transform([](const FullVertex3D &v) {
                               return Vertex3D{v.position};
                           }) |
                           std::ranges::to<std::vector>();

    auto [full_vertex_buffer, vertex_offset] =
        m_full_vertex_buffer.create_buffer(vertices.size());
    auto [index_buffer, first_index] =
        m_index_buffer.create_buffer(indices.size());
    auto vertex_buffer = m_vertex_buffer.create_buffer(vertices.size()).buffer;

    auto &mesh = m_meshes.emplace_back(
        vertex_buffer, full_vertex_buffer, index_buffer, material,
        indices.size(), vertex_offset, first_index, vertices.size());

    m_staging_buffer_manager->fill_buffer<Vertex3D>(
        std::span<const Vertex3D>(simple_vertices), *vertex_buffer,
        vertex_offset);

    m_staging_buffer_manager->fill_buffer<FullVertex3D>(
        vertices, *full_vertex_buffer, vertex_offset);

    m_staging_buffer_manager->fill_buffer<uint32_t>(indices, *index_buffer,
                                                    first_index);

    return mesh;
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
