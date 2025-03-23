#pragma once

#include "VulkanWrapper/Descriptors/Vertex.h"
#include "VulkanWrapper/Memory/Buffer.h"

namespace vw {
using FullVertex3DBuffer = Buffer<FullVertex3D, false, VertexBufferUsage>;
class Mesh {
  public:
    Mesh(const FullVertex3DBuffer *full_vertex_buffer,
         const IndexBuffer *index_buffer, vk::DescriptorSet descriptor_material,
         uint32_t indice_count, int vertex_offset, int first_index);

  private:
    const FullVertex3DBuffer *m_full_vertex_buffer;
    const IndexBuffer *m_index_buffer;
    vk::DescriptorSet m_descriptor_material;

    uint32_t m_indice_count;
    int m_vertex_offset;
    int m_first_index;
};
} // namespace vw
