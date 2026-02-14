#pragma once

#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>
#include <vector>

struct CameraConfig {
    glm::vec3 eye;
    glm::vec3 target;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    [[nodiscard]] glm::mat4 view_matrix() const {
        return glm::lookAt(eye, target, up);
    }
};

struct PendingInstance {
    size_t mesh_index;
    glm::mat4 transform;
};

struct SceneConfig {
    CameraConfig camera;
    std::vector<PendingInstance> instances;
};

/// Load meshes for a simple scene with a ground plane and a cube.
/// Returns camera config and pending instances to be added after upload.
inline SceneConfig
load_plane_with_cube_scene(vw::Model::MeshManager &mesh_manager) {
    SceneConfig config;

    // Load the ground plane
    mesh_manager.read_file("../../../Models/plane.obj");
    auto plane_mesh_count = mesh_manager.meshes().size();

    // Record plane instances with identity transform
    for (size_t i = 0; i < plane_mesh_count; ++i) {
        config.instances.push_back({i, glm::mat4(1.0f)});
    }

    // Load the cube
    mesh_manager.read_file("../../../Models/cube.obj");

    // Position cube above the plane
    glm::mat4 cube_transform = glm::mat4(1.0f);
    cube_transform =
        glm::translate(cube_transform, glm::vec3(0.0f, 50.0f, 0.0f));
    cube_transform = glm::scale(cube_transform, glm::vec3(30.0f));

    // Record cube instances
    for (size_t i = plane_mesh_count; i < mesh_manager.meshes().size();
         ++i) {
        config.instances.push_back({i, cube_transform});
    }

    // Camera looking at the scene from above and to the side
    config.camera = CameraConfig{
        .eye = glm::vec3(200.0f, 150.0f, 0.0f),
        .target = glm::vec3(0.0f, 100.0f, 0.0f),
    };
    return config;
}

/// Load meshes for the Sponza scene with a cube in the courtyard.
/// Returns camera config and pending instances to be added after upload.
inline SceneConfig
load_sponza_scene(vw::Model::MeshManager &mesh_manager) {
    SceneConfig config;

    // Load Sponza
    mesh_manager.read_file("../../../Models/Sponza/sponza.obj");
    auto sponza_mesh_count = mesh_manager.meshes().size();

    // Record all Sponza meshes with identity transform
    for (size_t i = 0; i < sponza_mesh_count; ++i) {
        config.instances.push_back({i, glm::mat4(1.0f)});
    }

    // Load the cube
    mesh_manager.read_file("../../../Models/cube.obj");

    // Create transform: scale by 200 and position in the center of Sponza
    glm::mat4 cube_transform = glm::mat4(1.0f);
    cube_transform =
        glm::translate(cube_transform, glm::vec3(0.0f, 200.0f, 50.0f));
    cube_transform = glm::scale(cube_transform, glm::vec3(200.0f));

    // Record cube instances
    for (size_t i = sponza_mesh_count; i < mesh_manager.meshes().size();
         ++i) {
        config.instances.push_back({i, cube_transform});
    }

    // Camera positioned to view curtains and lion head in Sponza
    config.camera = CameraConfig{
        .eye = glm::vec3(-900.0f, 300.0f, 100.0f),
        .target = glm::vec3(500.0f, 500.0f, 0.0f),
    };
    return config;
}

/// Add pending instances to the ray traced scene.
/// Call this after mesh upload so material addresses are resolved.
inline void add_instances(vw::rt::RayTracedScene &ray_traced_scene,
                          const vw::Model::MeshManager &mesh_manager,
                          const std::vector<PendingInstance> &instances) {
    for (const auto &inst : instances) {
        std::ignore = ray_traced_scene.add_instance(
            mesh_manager.meshes()[inst.mesh_index], inst.transform);
    }
}
