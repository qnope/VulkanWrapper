#pragma once

#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Descriptors/Vertex.h>
#include <VulkanWrapper/Memory/AllocateBufferUtils.h>
#include <VulkanWrapper/Memory/Buffer.h>
#include <VulkanWrapper/Model/Material/ColoredMaterialHandler.h>
#include <VulkanWrapper/Model/Material/MaterialData.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/Model/Primitive.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Vulkan/Queue.h>

struct CameraConfig {
    glm::vec3 eye;
    glm::vec3 target;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    [[nodiscard]] glm::mat4 view_matrix() const {
        return glm::lookAt(eye, target, up);
    }
};

struct UBOData {
    glm::mat4 proj = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 inverseViewProj = glm::mat4(1.0f);

    static UBOData create(float aspect_ratio,
                          const glm::mat4 &view_matrix) {
        UBOData data;
        data.proj = glm::perspective(glm::radians(60.0f),
                                     aspect_ratio, 0.1f, 100.f);
        data.proj[1][1] *= -1; // Flip Y for Vulkan
        data.view = view_matrix;
        data.inverseViewProj =
            glm::inverse(data.proj * data.view);
        return data;
    }
};

struct PendingInstance {
    size_t mesh_index;
    glm::mat4 transform;
};

struct SceneConfig {
    CameraConfig camera;
    std::vector<PendingInstance> instances;
    size_t plane_mesh_index;
    size_t cube_mesh_index;
};

/// Add a ModelPrimitiveResult to the MeshManager with a solid color
/// material. Returns the mesh index.
inline size_t
add_colored_mesh(vw::Model::MeshManager &mesh_manager,
                 vw::Model::ModelPrimitiveResult primitives,
                 glm::vec3 color) {
    using namespace vw::Model::Material;
    auto *handler = static_cast<ColoredMaterialHandler *>(
        mesh_manager.material_manager().handler(
            colored_material_tag));
    auto material =
        handler->create_material(ColoredMaterialData{color});
    mesh_manager.add_mesh(std::move(primitives.vertices),
                          std::move(primitives.indices), material);
    return mesh_manager.meshes().size() - 1;
}

/// Create the standard cube+plane scene with colored materials.
inline SceneConfig
create_cube_plane_scene(vw::Model::MeshManager &mesh_manager) {
    // Ground plane: white, scaled 20x
    auto plane_index = add_colored_mesh(
        mesh_manager,
        vw::Model::create_square(vw::Model::PlanePrimitive::XZ),
        glm::vec3(1.0f));

    // Cube: reddish color, translated above ground
    auto cube_index = add_colored_mesh(
        mesh_manager, vw::Model::create_cube(),
        glm::vec3(0.7f, 0.3f, 0.3f));

    SceneConfig config;
    config.plane_mesh_index = plane_index;
    config.cube_mesh_index = cube_index;

    // Ground plane transform: scale by 20x
    config.instances.push_back(
        {plane_index,
         glm::scale(glm::mat4(1.0f), glm::vec3(20.0f))});

    // Cube transform: translate up to (0, 1.5, 0)
    config.instances.push_back(
        {cube_index, glm::translate(glm::mat4(1.0f),
                                    glm::vec3(0.0f, 1.5f, 0.0f))});

    // Camera: positioned to see both cube and shadow
    config.camera = CameraConfig{
        .eye = glm::vec3(5.0f, 4.0f, 5.0f),
        .target = glm::vec3(0.0f, 0.5f, 0.0f),
    };

    return config;
}

/// Create and fill a UBO buffer with camera matrices.
inline auto create_ubo(const vw::Allocator &allocator,
                        float aspect_ratio,
                        const CameraConfig &camera) {
    auto buffer =
        vw::create_buffer<UBOData, true, vw::UniformBufferUsage>(
            allocator, 1);
    auto data = UBOData::create(aspect_ratio, camera.view_matrix());
    buffer.write(data, 0);
    return buffer;
}

/// Upload meshes, add instances, set SBT mapping, and build
/// acceleration structures.
inline void
setup_ray_tracing(vw::Model::MeshManager &mesh_manager,
                  vw::rt::RayTracedScene &ray_traced_scene,
                  vw::Device &device,
                  const std::vector<PendingInstance> &instances) {
    // Upload mesh data to GPU
    auto cmd = mesh_manager.fill_command_buffer();
    device.graphicsQueue().enqueue_command_buffer(cmd);
    device.graphicsQueue().submit({}, {}, {}).wait();

    // Add instances to ray traced scene
    for (const auto &inst : instances) {
        std::ignore = ray_traced_scene.add_instance(
            mesh_manager.meshes()[inst.mesh_index],
            inst.transform);
    }

    // Set per-material SBT offsets
    ray_traced_scene.set_material_sbt_mapping(
        mesh_manager.material_manager().sbt_mapping());

    // Build acceleration structures
    ray_traced_scene.build();
}
