#include "VulkanWrapper/Model/Importer.h"

#include "VulkanWrapper/Model/Internal/Material.h"
#include "VulkanWrapper/Model/Internal/Mesh.h"
#include "VulkanWrapper/Model/Material.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/Utils/Algos.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace vw::Model {
void import_model(const std::filesystem::path &path,
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

    auto scene = importer.ReadFile(path.string().c_str(), post_process);

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

            return mesh_manager.m_staging_buffer_manager.stage_image_from_path(
                "../../Models/Sponza/" + material.diffuse_texture_path.string(),
                true);
        }();

        DescriptorAllocator allocator;
        allocator.add_combined_image(0, image);
        auto set = mesh_manager.m_descriptor_pool.allocate_set(allocator);
        real_material.emplace_back(image, set);
    }

    for (const auto &mesh : meshes) {
        auto [vertex_buffer, vertex_offset] =
            mesh_manager.m_vertex_buffer.create_buffer(mesh.vertices.size());
        auto [index_buffer, first_index] =
            mesh_manager.m_index_buffer.create_buffer(mesh.indices.size());

        mesh_manager.m_meshes.emplace_back(
            vertex_buffer, index_buffer, real_material[mesh.material_index],
            mesh.indices.size(), vertex_offset, first_index);

        mesh_manager.m_staging_buffer_manager.fill_buffer<FullVertex3D>(
            mesh.vertices, *vertex_buffer, vertex_offset);

        mesh_manager.m_staging_buffer_manager.fill_buffer<uint32_t>(
            mesh.indices, *index_buffer, first_index);
    }
}
} // namespace vw::Model
