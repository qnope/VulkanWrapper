#include "VulkanWrapper/Model/MeshManager.h"

#include "VulkanWrapper/Model/Importer.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialHandler.h"
#include "VulkanWrapper/Model/Material/TexturedMaterialHandler.h"

namespace vw::Model {

MeshManager::MeshManager(std::shared_ptr<const Device> device,
                         std::shared_ptr<Allocator> allocator)
    : m_staging_buffer_manager{
          std::make_shared<StagingBufferManager>(device, allocator)}
    , m_vertex_buffer{allocator}
    , m_full_vertex_buffer{allocator}
    , m_index_buffer{allocator}
    , m_material_manager{device, allocator, m_staging_buffer_manager} {
    m_material_manager.register_handler<Material::TexturedMaterialHandler>();
    m_material_manager.register_handler<Material::ColoredMaterialHandler>();
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
