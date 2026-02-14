#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/Vertex.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Model/Material/Material.h"

namespace vw::Model {
using Vertex3DBuffer = Buffer<Vertex3D, false, VertexBufferUsage>;
using FullVertex3DBuffer = Buffer<FullVertex3D, false, VertexBufferUsage>;

/// Push constants for mesh rendering with buffer reference materials.
struct MeshPushConstants {
    glm::mat4 transform;
    vk::DeviceAddress material_address;
};

class Mesh {
  public:
    Mesh(std::shared_ptr<const Vertex3DBuffer> vertex_buffer,
         std::shared_ptr<const FullVertex3DBuffer> full_vertex_buffer,
         std::shared_ptr<const IndexBuffer> index_buffer,
         Material::Material material, uint32_t indice_count, int vertex_offset,
         int first_index, int vertices_count);

    [[nodiscard]] Material::MaterialTypeTag material_type_tag() const noexcept;

    /// Draw the mesh using push constants with buffer device address.
    void draw(vk::CommandBuffer cmd_buffer,
              const PipelineLayout &pipeline_layout,
              const glm::mat4 &transform) const;

    void draw_zpass(vk::CommandBuffer cmd_buffer,
                    const PipelineLayout &pipeline_layout,
                    const glm::mat4 &transform) const;

    [[nodiscard]] vk::AccelerationStructureGeometryKHR
    acceleration_structure_geometry() const noexcept;

    [[nodiscard]] vk::AccelerationStructureBuildRangeInfoKHR
    acceleration_structure_range_info() const noexcept;

    [[nodiscard]] uint32_t index_count() const noexcept {
        return m_indice_count;
    }

    [[nodiscard]] const Material::Material &material() const noexcept {
        return m_material;
    }

    [[nodiscard]] std::shared_ptr<const FullVertex3DBuffer>
    full_vertex_buffer() const noexcept {
        return m_full_vertex_buffer;
    }

    [[nodiscard]] std::shared_ptr<const IndexBuffer>
    index_buffer() const noexcept {
        return m_index_buffer;
    }

    [[nodiscard]] int vertex_offset() const noexcept { return m_vertex_offset; }

    [[nodiscard]] int first_index() const noexcept { return m_first_index; }

    /// Returns a hash of this mesh's geometry.
    /// Meshes with the same geometry_hash() share identical triangle data.
    [[nodiscard]] size_t geometry_hash() const noexcept;

    /// Equality operator comparing geometry identity (not materials).
    /// Two meshes are equal if they reference the same GPU geometry.
    bool operator==(const Mesh &other) const noexcept;

  private:
    std::shared_ptr<const Vertex3DBuffer> m_vertex_buffer;
    std::shared_ptr<const FullVertex3DBuffer> m_full_vertex_buffer;
    std::shared_ptr<const IndexBuffer> m_index_buffer;
    Material::Material m_material;

    uint32_t m_indice_count;
    int m_vertex_offset;
    int m_first_index;
    int m_vertices_count;
};
} // namespace vw::Model

template <> struct std::hash<vw::Model::Mesh> {
    size_t operator()(const vw::Model::Mesh &mesh) const noexcept {
        return mesh.geometry_hash();
    }
};
