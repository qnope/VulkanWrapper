#include "VulkanWrapper/Model/Mesh.h"

#include "VulkanWrapper/Pipeline/PipelineLayout.h"

namespace vw ::Model {
Mesh::Mesh(const Vertex3DBuffer *vertex_buffer,
           const FullVertex3DBuffer *full_vertex_buffer,
           const IndexBuffer *index_buffer, Material::Material material,
           uint32_t indice_count, int vertex_offset, int first_index)
    : m_vertex_buffer{vertex_buffer}
    , m_full_vertex_buffer{full_vertex_buffer}
    , m_index_buffer{index_buffer}
    , m_material{material}
    , m_indice_count{indice_count}
    , m_vertex_offset{vertex_offset}
    , m_first_index{first_index} {}

Material::MaterialTypeTag Mesh::material_type_tag() const noexcept {
    return m_material.material_type;
}

void Mesh::draw(vk::CommandBuffer cmd_buffer, const PipelineLayout &layout,
                uint32_t material_descriptor_set_index) const {
    vk::Buffer vb = m_full_vertex_buffer->handle();
    vk::Buffer ib = m_index_buffer->handle();
    vk::DeviceSize vbo = 0;
    cmd_buffer.bindVertexBuffers(0, vb, vbo);
    cmd_buffer.bindIndexBuffer(ib, 0, vk::IndexType::eUint32);
    cmd_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, layout.handle(),
        material_descriptor_set_index, m_material.descriptor_set, nullptr);
    cmd_buffer.drawIndexed(m_indice_count, 1, m_first_index, m_vertex_offset,
                           0);
}

void Mesh::draw_zpass(vk::CommandBuffer cmd_buffer) const {
    vk::Buffer vb = m_vertex_buffer->handle();
    vk::Buffer ib = m_index_buffer->handle();
    vk::DeviceSize vbo = 0;
    cmd_buffer.bindVertexBuffers(0, vb, vbo);
    cmd_buffer.bindIndexBuffer(ib, 0, vk::IndexType::eUint32);
    cmd_buffer.drawIndexed(m_indice_count, 1, m_first_index, m_vertex_offset,
                           0);
}

vk::AccelerationStructureGeometryKHR
Mesh::acceleration_structure_geometry() const noexcept {
    // Create triangle data for acceleration structure
    vk::AccelerationStructureGeometryTrianglesDataKHR triangles;
    triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat)
        .setVertexData(vk::DeviceOrHostAddressConstKHR{
            m_vertex_buffer->device_address() +
            m_vertex_offset * sizeof(vw::Vertex3D)})
        .setVertexStride(sizeof(vw::Vertex3D))
        .setMaxVertex(static_cast<uint32_t>(
            m_vertex_buffer->size() / sizeof(vw::Vertex3D) - 1))
        .setIndexType(vk::IndexType::eUint32)
        .setIndexData(
            vk::DeviceOrHostAddressConstKHR{m_index_buffer->device_address() +
                                            sizeof(uint32_t) * m_first_index})
        .setTransformData(
            vk::DeviceOrHostAddressConstKHR{}); // No transform buffer

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

uint32_t Mesh::index_count() const noexcept { return m_indice_count; }

} // namespace vw::Model
