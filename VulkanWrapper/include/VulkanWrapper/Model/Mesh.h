#pragma once

#include "VulkanWrapper/Descriptors/Vertex.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Model/Material.h"

namespace vw::Model {
using FullVertex3DBuffer = Buffer<FullVertex3D, false, VertexBufferUsage>;
class Mesh {
  public:
    Mesh(const FullVertex3DBuffer *full_vertex_buffer,
         const IndexBuffer *index_buffer, Material descriptor_material,
         uint32_t indice_count, int vertex_offset, int first_index);

    void draw(vk::CommandBuffer cmd_buffer,
              const PipelineLayout &pipeline_layout) const;

  private:
    const FullVertex3DBuffer *m_full_vertex_buffer;
    const IndexBuffer *m_index_buffer;
    Material m_material;

    uint32_t m_indice_count;
    int m_vertex_offset;
    int m_first_index;
};
} // namespace vw::Model
