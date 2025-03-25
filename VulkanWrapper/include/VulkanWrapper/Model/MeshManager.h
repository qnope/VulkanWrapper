#pragma once

#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Descriptors/Vertex.h"
#include "VulkanWrapper/Memory/BufferList.h"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Model/Mesh.h"

namespace vw::Model {
class MeshManager {
    friend void import_model(const std::filesystem::path &, MeshManager &);

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
    BufferList<FullVertex3D, false, VertexBufferUsage> m_vertex_buffer;
    IndexBufferList m_index_buffer;

    std::vector<Mesh> m_meshes;
};
} // namespace vw::Model
