#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/Descriptors/Vertex.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/BufferList.h"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Model/Material/MaterialFactory.h"
#include "VulkanWrapper/Model/Material/MaterialManagerMap.h"
#include "VulkanWrapper/Model/Mesh.h"

namespace vw::Model {
class MeshManager {
    friend void import_model(const std::filesystem::path &, MeshManager &);

  public:
    MeshManager(std::shared_ptr<const Device> device,
                std::shared_ptr<const Allocator> allocator);

    void read_file(const std::filesystem::path &path);

    [[nodiscard]] vk::CommandBuffer fill_command_buffer();

    [[nodiscard]] const std::vector<Mesh> &meshes() const noexcept;

    [[nodiscard]] const Material::MaterialManagerMap &
    material_manager_map() const noexcept;

  private:
    void create_default_material_managers(std::shared_ptr<const Device> device,
                                          std::shared_ptr<const Allocator> allocator);
    void create_default_material_factories();

  private:
    std::shared_ptr<StagingBufferManager> m_staging_buffer_manager;
    BufferList<Vertex3D, false, VertexBufferUsage> m_vertex_buffer;
    BufferList<FullVertex3D, false, VertexBufferUsage> m_full_vertex_buffer;
    IndexBufferList m_index_buffer;
    std::shared_ptr<Material::MaterialManagerMap> m_material_manager_map;
    Material::MaterialFactory m_material_factory;
    std::vector<Mesh> m_meshes;
};
} // namespace vw::Model
