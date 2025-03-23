#include "VulkanWrapper/Model/Mesh.h"

namespace vw {
Mesh::Mesh(const FullVertex3DBuffer *full_vertex_buffer,
           const IndexBuffer *index_buffer,
           vk::DescriptorSet descriptor_material, uint32_t indice_count,
           int vertex_offset, int first_index)
    : m_full_vertex_buffer{full_vertex_buffer}
    , m_index_buffer{index_buffer}
    , m_descriptor_material{descriptor_material}
    , m_indice_count{indice_count}
    , m_vertex_offset{vertex_offset}
    , m_first_index{first_index} {}
} // namespace vw
