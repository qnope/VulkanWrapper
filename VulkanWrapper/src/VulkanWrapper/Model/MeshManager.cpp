#include "VulkanWrapper/Model/MeshManager.h"

#include "VulkanWrapper/Model/Importer.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialManager.h"
#include "VulkanWrapper/Model/Material/TexturedMaterialManager.h"

namespace vw::Model {

MeshManager::MeshManager(const Device &device, const Allocator &allocator)
    : m_staging_buffer_manager{device, allocator}
    , m_vertex_buffer{allocator}
    , m_index_buffer{allocator} {
    create_default_material_managers(device, allocator);
}

void MeshManager::read_file(const std::filesystem::path &path) {
    import_model(path, *this);
}

vk::CommandBuffer MeshManager::fill_command_buffer() {
    return m_staging_buffer_manager.fill_command_buffer();
}

const std::vector<Mesh> &MeshManager::meshes() const noexcept {
    return m_meshes;
}

const Material::MaterialManagerMap &
MeshManager::material_manager_map() const noexcept {
    return m_material_manager_map;
}

void MeshManager::create_default_material_managers(const Device &device,
                                                   const Allocator &allocator) {
    m_material_manager_map.insert_manager(
        std::make_unique<Material::TextureMaterialManager>(
            device, m_staging_buffer_manager));
    m_material_manager_map.insert_manager(
        std::make_unique<Material::ColoredMaterialManager>(
            device, allocator, m_staging_buffer_manager));
}
} // namespace vw::Model
