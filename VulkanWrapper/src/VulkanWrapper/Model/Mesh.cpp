#include "VulkanWrapper/Model/Mesh.h"

#include "VulkanWrapper/Pipeline/PipelineLayout.h"

namespace vw ::Model {
Mesh::Mesh(std::shared_ptr<const Vertex3DBuffer> vertex_buffer,
           std::shared_ptr<const FullVertex3DBuffer> full_vertex_buffer,
           std::shared_ptr<const IndexBuffer> index_buffer,
           Material::Material material, uint32_t indice_count,
           int vertex_offset, int first_index, int vertices_count)
    : m_vertex_buffer{std::move(vertex_buffer)}
    , m_full_vertex_buffer{std::move(full_vertex_buffer)}
    , m_index_buffer{std::move(index_buffer)}
    , m_material{material}
    , m_indice_count{indice_count}
    , m_vertex_offset{vertex_offset}
    , m_first_index{first_index}
    , m_vertices_count{vertices_count} {}

Material::MaterialTypeTag Mesh::material_type_tag() const noexcept {
    return m_material.material_type;
}

void Mesh::draw(vk::CommandBuffer cmd_buffer, const PipelineLayout &layout,
                const glm::mat4 &transform) const {
    vk::Buffer vb = m_full_vertex_buffer->handle();
    vk::Buffer ib = m_index_buffer->handle();
    vk::DeviceSize vbo = 0;
    cmd_buffer.bindVertexBuffers(0, vb, vbo);
    cmd_buffer.bindIndexBuffer(ib, 0, vk::IndexType::eUint32);

    MeshPushConstants push{};
    push.transform = transform;
    push.material_index = m_material.material_index;

    cmd_buffer.pushConstants(
        layout.handle(),
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0, sizeof(MeshPushConstants), &push);
    cmd_buffer.drawIndexed(m_indice_count, 1, m_first_index, m_vertex_offset,
                           0);
}

void Mesh::draw_zpass(vk::CommandBuffer cmd_buffer,
                      const PipelineLayout &layout,
                      const glm::mat4 &transform) const {
    vk::Buffer vb = m_vertex_buffer->handle();
    vk::Buffer ib = m_index_buffer->handle();
    vk::DeviceSize vbo = 0;
    cmd_buffer.bindVertexBuffers(0, vb, vbo);
    cmd_buffer.bindIndexBuffer(ib, 0, vk::IndexType::eUint32);
    cmd_buffer.pushConstants(layout.handle(), vk::ShaderStageFlagBits::eVertex,
                             0, sizeof(glm::mat4), &transform);
    cmd_buffer.drawIndexed(m_indice_count, 1, m_first_index, m_vertex_offset,
                           0);
}

vk::AccelerationStructureGeometryKHR
Mesh::acceleration_structure_geometry() const noexcept {
    // Create triangle data for acceleration structure
    vk::AccelerationStructureGeometryTrianglesDataKHR triangles;
    triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat)
        .setVertexData(m_vertex_buffer->device_address() +
                       sizeof(vw::Vertex3D) * m_vertex_offset)
        .setVertexStride(sizeof(vw::Vertex3D))
        .setMaxVertex(m_vertices_count - 1)
        .setIndexType(vk::IndexType::eUint32)
        .setIndexData(m_index_buffer->device_address() +
                      sizeof(uint32_t) * m_first_index);

    // Create geometry data
    vk::AccelerationStructureGeometryDataKHR geometryData;
    geometryData.setTriangles(triangles);

    // Create acceleration structure geometry
    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles)
        .setGeometry(geometryData)
        .setFlags(vk::GeometryFlagBitsKHR::eOpaque);

    return geometry;
}

vk::AccelerationStructureBuildRangeInfoKHR
Mesh::acceleration_structure_range_info() const noexcept {
    return vk::AccelerationStructureBuildRangeInfoKHR().setPrimitiveCount(
        m_indice_count / 3);
}

size_t Mesh::geometry_hash() const noexcept {
    // FNV-1a style hash combining
    size_t h = 14695981039346656037ULL;
    auto combine = [&h](auto value) {
        h ^= static_cast<size_t>(value);
        h *= 1099511628211ULL;
    };
    combine(m_vertex_buffer->device_address());
    combine(m_vertex_offset);
    combine(m_vertices_count);
    combine(m_index_buffer->device_address());
    combine(m_first_index);
    combine(m_indice_count);
    return h;
}

bool Mesh::operator==(const Mesh &other) const noexcept {
    return m_vertex_buffer->device_address() ==
               other.m_vertex_buffer->device_address() &&
           m_vertex_offset == other.m_vertex_offset &&
           m_vertices_count == other.m_vertices_count &&
           m_index_buffer->device_address() ==
               other.m_index_buffer->device_address() &&
           m_first_index == other.m_first_index &&
           m_indice_count == other.m_indice_count;
}

} // namespace vw::Model
