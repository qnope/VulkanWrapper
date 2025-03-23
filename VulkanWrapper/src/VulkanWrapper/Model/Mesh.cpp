#include "VulkanWrapper/Model/Mesh.h"

#include "VulkanWrapper/Pipeline/PipelineLayout.h"

namespace vw ::Model {
Mesh::Mesh(const FullVertex3DBuffer *full_vertex_buffer,
           const IndexBuffer *index_buffer, Material material,
           uint32_t indice_count, int vertex_offset, int first_index)
    : m_full_vertex_buffer{full_vertex_buffer}
    , m_index_buffer{index_buffer}
    , m_material{material}
    , m_indice_count{indice_count}
    , m_vertex_offset{vertex_offset}
    , m_first_index{first_index} {}

void Mesh::draw(vk::CommandBuffer cmd_buffer,
                const PipelineLayout &layout) const {
    vk::Buffer vb = m_full_vertex_buffer->handle();
    vk::Buffer ib = m_index_buffer->handle();
    vk::DeviceSize vbo = 0;
    cmd_buffer.bindVertexBuffers(0, vb, vbo);
    cmd_buffer.bindIndexBuffer(ib, 0, vk::IndexType::eUint32);
    cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                  layout.handle(), 1, m_material.descriptor_set,
                                  nullptr);
    cmd_buffer.drawIndexed(m_indice_count, 1, m_first_index, m_vertex_offset,
                           0);
}
} // namespace vw::Model
