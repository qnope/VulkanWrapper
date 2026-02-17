#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/Vertex.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/BufferList.h"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Model/Material/BindlessMaterialManager.h"
#include "VulkanWrapper/Model/Mesh.h"

namespace vw::Model {

class MeshManager {
    friend void import_model(const std::filesystem::path &, MeshManager &);

  public:
    MeshManager(std::shared_ptr<const Device> device,
                std::shared_ptr<Allocator> allocator);

    void read_file(const std::filesystem::path &path);

    const Mesh &add_mesh(std::span<const FullVertex3D> vertices,
                         std::span<const uint32_t> indices,
                         Material::Material material);

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
