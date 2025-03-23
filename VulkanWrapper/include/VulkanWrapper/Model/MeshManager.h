#pragma once

#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Descriptors/Vertex.h"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Model/Material.h"
#include "VulkanWrapper/Model/Mesh.h"

namespace vw::Model {
class MeshManager {
    friend class Importer;

  public:
    MeshManager(const Device &device, const Allocator &allocator);

    [[nodiscard]] std::shared_ptr<DescriptorSetLayout> layout() const noexcept;

    void read_file(const std::filesystem::path &path);

    vk::CommandBuffer fill_command_buffer();

    [[nodiscard]] const std::vector<Mesh> &meshes() const noexcept;

  private:
    StagingBufferManager m_staging_buffer_manager;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_set_layout;
    DescriptorPool m_descriptor_pool;
    Buffer<FullVertex3D, false, VertexBufferUsage> m_vertex_buffer;
    IndexBuffer m_index_buffer;
    int m_vertex_offset = 0;
    int m_index_offset = 0;

    std::vector<Mesh> m_meshes;
};
} // namespace vw::Model
