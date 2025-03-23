#include "VulkanWrapper/Model/Importer.h"

#include "VulkanWrapper/Model/Internal/Material.h"
#include "VulkanWrapper/Model/Internal/Mesh.h"
#include "VulkanWrapper/Model/Material.h"
#include "VulkanWrapper/Model/Mesh.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/Utils/Algos.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace vw::Model {
Importer::Importer(const std::filesystem::path &path,
                   MeshManager &mesh_manager) {
    Assimp::Importer importer;
    constexpr auto post_process =
        aiPostProcessSteps::aiProcess_Triangulate |
        aiPostProcessSteps::aiProcess_FlipUVs |
        aiPostProcessSteps::aiProcess_GenUVCoords |
        aiPostProcessSteps::aiProcess_OptimizeGraph |
        aiPostProcessSteps::aiProcess_SplitLargeMeshes |
        aiPostProcessSteps::aiProcess_CalcTangentSpace |
        aiPostProcessSteps::aiProcess_GenSmoothNormals |
        aiPostProcessSteps::aiProcess_ImproveCacheLocality |
        aiPostProcessSteps::aiProcess_JoinIdenticalVertices |
        aiPostProcessSteps::aiProcess_RemoveRedundantMaterials;

    auto scene = importer.ReadFile(path.c_str(), post_process);

    if (scene == nullptr) {
        throw ModelNotFoundException{std::source_location::current()};
    }

    std::vector<Internal::Mesh> meshes =
        std::span(scene->mMeshes, scene->mNumMeshes) |
        construct<Internal::Mesh> | to<std::vector>;

    std::vector<Internal::Material> materials =
        std::span(scene->mMaterials, scene->mNumMaterials) |
        construct<Internal::Material> | to<std::vector>;

    std::vector<Material> real_material;

    for (const auto &material : materials) {
        auto image = [&] {
            if (material.diffuse_texture_path.empty())
                return mesh_manager.m_staging_buffer_manager
                    .stage_image_from_path("../../Images/image_test.png", true);
            else
                return mesh_manager.m_staging_buffer_manager
                    .stage_image_from_path(
                        std::string("../../Models/Sponza/") +
                            material.diffuse_texture_path.c_str(),
                        true);
        }();

        DescriptorAllocator allocator;
        allocator.add_combined_image(0, image);
        auto set = mesh_manager.m_descriptor_pool.allocate_set(allocator);
        real_material.emplace_back(image, set);
    }

    for (const auto &mesh : meshes) {
        mesh_manager.m_meshes.emplace_back(
            &mesh_manager.m_vertex_buffer, &mesh_manager.m_index_buffer,
            real_material[mesh.material_index], mesh.indices.size(),
            mesh_manager.m_vertex_offset, mesh_manager.m_index_offset);

        mesh_manager.m_staging_buffer_manager.fill_buffer<FullVertex3D>(
            mesh.vertices, mesh_manager.m_vertex_buffer,
            mesh_manager.m_vertex_offset);

        mesh_manager.m_staging_buffer_manager.fill_buffer<uint32_t>(
            mesh.indices, mesh_manager.m_index_buffer,
            mesh_manager.m_index_offset);

        mesh_manager.m_index_offset += mesh.indices.size();
        mesh_manager.m_vertex_offset += mesh.vertices.size();
    }
    std::cout << "lol" << std::endl;
}
} // namespace vw::Model
