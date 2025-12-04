#include "VulkanWrapper/Model/MeshManager.h"

#include "VulkanWrapper/Model/Importer.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialManager.h"
#include "VulkanWrapper/Model/Material/TexturedMaterialManager.h"

namespace vw::Model {

MeshManager::MeshManager(std::shared_ptr<const Device> device,
                         std::shared_ptr<const Allocator> allocator)
    : m_staging_buffer_manager{std::make_shared<StagingBufferManager>(device, allocator)}
    , m_vertex_buffer{allocator}
    , m_full_vertex_buffer{allocator}
    , m_index_buffer{allocator}
    , m_material_manager_map{std::make_shared<Material::MaterialManagerMap>()}
    , m_material_factory{m_material_manager_map} {
    create_default_material_managers(device, allocator);
    create_default_material_factories();
}

void MeshManager::read_file(const std::filesystem::path &path) {
    import_model(path, *this);
}

vk::CommandBuffer MeshManager::fill_command_buffer() {
    return m_staging_buffer_manager->fill_command_buffer();
}

const std::vector<Mesh> &MeshManager::meshes() const noexcept {
    return m_meshes;
}

const Material::MaterialManagerMap &
MeshManager::material_manager_map() const noexcept {
    return *m_material_manager_map;
}

void MeshManager::create_default_material_managers(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const Allocator> allocator) {
    m_material_manager_map->insert_manager(
        std::make_unique<Material::TextureMaterialManager>(
            device, m_staging_buffer_manager));

    m_material_manager_map->insert_manager(
        std::make_unique<Material::ColoredMaterialManager>(
            device, allocator, m_staging_buffer_manager));
}

void MeshManager::create_default_material_factories() {
    using namespace Material;
    m_material_factory.insert_factory(colored_material_priority,
                                      std::function(allocate_colored_material));
    m_material_factory.insert_factory(
        textured_material_priority, std::function(allocate_textured_material));
}
} // namespace vw::Model
