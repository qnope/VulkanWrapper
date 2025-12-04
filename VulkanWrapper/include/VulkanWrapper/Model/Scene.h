#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Model/Mesh.h"

namespace vw::Model {

/// Represents an instance of a mesh in the scene with its own transformation.
/// The mesh is copied (cheap: shared_ptr members + small vector in DescriptorSet).
struct MeshInstance {
    Mesh mesh;
    glm::mat4 transform = glm::mat4(1.0f);
};

/// Scene class that holds mesh instances, allowing the same mesh to be reused
/// with different transformations
class Scene {
  public:
    Scene() = default;

    /// Add a mesh instance with identity transform
    void add_mesh_instance(const Mesh &mesh) {
        m_instances.push_back(MeshInstance{mesh, glm::mat4(1.0f)});
    }

    /// Add a mesh instance with a specific transform
    void add_mesh_instance(const Mesh &mesh, const glm::mat4 &transform) {
        m_instances.push_back(MeshInstance{mesh, transform});
    }

    /// Get all mesh instances
    [[nodiscard]] const std::vector<MeshInstance> &instances() const noexcept {
        return m_instances;
    }

    /// Get mutable access to instances for modifying transforms
    [[nodiscard]] std::vector<MeshInstance> &instances() noexcept {
        return m_instances;
    }

    /// Clear all instances
    void clear() noexcept { m_instances.clear(); }

    /// Get the number of instances
    [[nodiscard]] size_t size() const noexcept { return m_instances.size(); }

  private:
    std::vector<MeshInstance> m_instances;
};

} // namespace vw::Model
