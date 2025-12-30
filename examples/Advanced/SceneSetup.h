#pragma once

#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>

struct CameraConfig {
    glm::vec3 eye;
    glm::vec3 target;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    [[nodiscard]] glm::mat4 view_matrix() const {
        return glm::lookAt(eye, target, up);
    }
};

/// Setup a simple scene with a ground plane and a cube floating above it.
/// Returns the camera configuration for this scene.
inline CameraConfig
setup_plane_with_cube_scene(vw::Model::MeshManager &mesh_manager,
                            vw::rt::RayTracedScene &ray_traced_scene) {
    // Load the ground plane
    mesh_manager.read_file("../../../Models/plane.obj");
    auto plane_mesh_count = mesh_manager.meshes().size();

    // Add plane with identity transform (it's already at Y=0)
    for (size_t i = 0; i < plane_mesh_count; ++i) {
        std::ignore = ray_traced_scene.add_instance(mesh_manager.meshes()[i],
                                                    glm::mat4(1.0f));
    }

    // Load the cube
    mesh_manager.read_file("../../../Models/cube.obj");

    // Position cube above the plane
    glm::mat4 cube_transform = glm::mat4(1.0f);
    cube_transform =
        glm::translate(cube_transform, glm::vec3(0.0f, 50.0f, 0.0f));
    cube_transform = glm::scale(cube_transform, glm::vec3(30.0f));

    // Add cube mesh
    for (size_t i = plane_mesh_count; i < mesh_manager.meshes().size(); ++i) {
        std::ignore = ray_traced_scene.add_instance(mesh_manager.meshes()[i],
                                                    cube_transform);
    }

    // Camera looking at the scene from above and to the side
    return CameraConfig{
        .eye = glm::vec3(200.0f, 150.0f, 0.0f),
        .target = glm::vec3(0.0f, 100.0f, 0.0f),
    };
}

/// Setup the Sponza scene with a cube in the courtyard.
/// Returns the camera configuration for this scene.
inline CameraConfig
setup_sponza_scene(vw::Model::MeshManager &mesh_manager,
                   vw::rt::RayTracedScene &ray_traced_scene) {
    // Load Sponza
    mesh_manager.read_file("../../../Models/Sponza/sponza.obj");
    auto sponza_mesh_count = mesh_manager.meshes().size();

    // Add all Sponza meshes with identity transform
    for (size_t i = 0; i < sponza_mesh_count; ++i) {
        std::ignore = ray_traced_scene.add_instance(mesh_manager.meshes()[i],
                                                    glm::mat4(1.0f));
    }

    // Load and add a cube in the middle of the scene
    mesh_manager.read_file("../../../Models/cube.obj");

    // Create transform: scale by 200 and position in the center of Sponza
    // Sponza's courtyard is roughly at origin, raise cube above ground
    glm::mat4 cube_transform = glm::mat4(1.0f);
    cube_transform =
        glm::translate(cube_transform, glm::vec3(0.0f, 200.0f, 50.0f));
    cube_transform = glm::scale(cube_transform, glm::vec3(200.0f));

    // Add cube mesh
    for (size_t i = sponza_mesh_count; i < mesh_manager.meshes().size(); ++i) {
        std::ignore = ray_traced_scene.add_instance(mesh_manager.meshes()[i],
                                                    cube_transform);
    }

    // Camera positioned to view curtains and lion head in Sponza
    return CameraConfig{
        .eye = glm::vec3(-900.0f, 300.0f, 100.0f),
        .target = glm::vec3(500.0f, 800.0f, 0.0f),
    };
}
