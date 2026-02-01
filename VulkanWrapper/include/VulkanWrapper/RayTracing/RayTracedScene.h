#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Model/Scene.h"
#include "VulkanWrapper/RayTracing/BottomLevelAccelerationStructure.h"
#include "VulkanWrapper/RayTracing/GeometryReference.h"
#include "VulkanWrapper/RayTracing/TopLevelAccelerationStructure.h"
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace vw::rt {

struct InstanceId {
    uint32_t value;

    bool operator==(const InstanceId &other) const noexcept = default;
};

class RayTracedScene {
  public:
    RayTracedScene(std::shared_ptr<const Device> device,
                   std::shared_ptr<const Allocator> allocator);

    RayTracedScene(const RayTracedScene &) = delete;
    RayTracedScene &operator=(const RayTracedScene &) = delete;
    RayTracedScene(RayTracedScene &&) = default;
    RayTracedScene &operator=(RayTracedScene &&) = default;

    /// Add an instance of a mesh. The mesh geometry is automatically
    /// registered if not already known (deduplication via geometry hash).
    /// This also adds the mesh to the embedded Scene for rasterization.
    [[nodiscard]] InstanceId
    add_instance(const Model::Mesh &mesh,
                 const glm::mat4 &transform = glm::mat4(1.0f));

    void set_transform(InstanceId instance_id, const glm::mat4 &transform);

    [[nodiscard]] const glm::mat4 &get_transform(InstanceId instance_id) const;

    void set_visible(InstanceId instance_id, bool visible);

    [[nodiscard]] bool is_visible(InstanceId instance_id) const;

    void set_sbt_offset(InstanceId instance_id, uint32_t offset);

    [[nodiscard]] uint32_t get_sbt_offset(InstanceId instance_id) const;

    void set_custom_index(InstanceId instance_id, uint32_t custom_index);

    [[nodiscard]] uint32_t get_custom_index(InstanceId instance_id) const;

    void remove_instance(InstanceId instance_id);

    [[nodiscard]] bool is_valid(InstanceId instance_id) const;

    void build();

    void update();

    [[nodiscard]] bool needs_build() const noexcept;

    [[nodiscard]] bool needs_update() const noexcept;

    [[nodiscard]] vk::DeviceAddress tlas_device_address() const;

    [[nodiscard]] vk::AccelerationStructureKHR tlas_handle() const;

    [[nodiscard]] const as::TopLevelAccelerationStructure &tlas() const;

    [[nodiscard]] vk::DeviceAddress geometry_buffer_address() const;

    [[nodiscard]] const GeometryReferenceBuffer &geometry_buffer() const;

    [[nodiscard]] bool has_geometry_buffer() const noexcept;

    [[nodiscard]] size_t mesh_count() const noexcept;

    [[nodiscard]] size_t instance_count() const noexcept;

    [[nodiscard]] size_t visible_instance_count() const noexcept;

    /// Access the embedded Scene for rasterization rendering.
    [[nodiscard]] const Model::Scene &scene() const noexcept { return m_scene; }

    /// Access the embedded Scene for rasterization rendering (mutable).
    [[nodiscard]] Model::Scene &scene() noexcept { return m_scene; }

  private:
    struct Instance {
        uint32_t blas_index;
        glm::mat4 transform = glm::mat4(1.0f);
        bool visible = true;
        bool active = true;
        uint32_t sbt_offset = 0;
        uint32_t custom_index = 0;
    };

    struct MeshGeometry {
        std::shared_ptr<const Model::FullVertex3DBuffer> full_vertex_buffer;
        std::shared_ptr<const IndexBuffer> index_buffer;
        int vertex_offset;
        int first_index;
        Model::Material::Material material;
        glm::mat4 matrix;
    };

    uint32_t get_or_create_blas_index(const Model::Mesh &mesh);
    void build_blas();
    void build_tlas();
    void build_geometry_buffer();

    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Allocator> m_allocator;

    std::vector<Instance> m_instances;

    std::optional<as::BottomLevelAccelerationStructureList> m_blas_list;
    std::optional<as::TopLevelAccelerationStructure> m_tlas;

    bool m_blas_dirty = false;
    bool m_tlas_dirty = false;

    // Geometry deduplication: maps mesh -> BLAS index
    std::unordered_map<Model::Mesh, uint32_t> m_mesh_to_blas_index;

    // Geometry data for each BLAS index (ordered by BLAS index)
    std::vector<MeshGeometry> m_mesh_geometries;

    // Geometry reference buffer for ray tracing shaders
    std::optional<GeometryReferenceBuffer> m_geometry_buffer;

    // Embedded scene for rasterization
    Model::Scene m_scene;
};

} // namespace vw::rt
