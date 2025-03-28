#include "VulkanWrapper/Model/Importer.h"

#include "VulkanWrapper/Model/Internal/MaterialInfo.h"
#include "VulkanWrapper/Model/Internal/MeshInfo.h"
#include "VulkanWrapper/Model/Material/Material.h"
#include "VulkanWrapper/Model/Material/TexturedMaterialManager.h"
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

    const auto *scene = importer.ReadFile(path.string().c_str(), post_process);

    if (scene == nullptr) {
        throw ModelNotFoundException{std::source_location::current()};
    }

    std::vector<Internal::MeshInfo> meshes =
        std::span(scene->mMeshes, scene->mNumMeshes) |
        construct<Internal::MeshInfo> | to<std::vector>;

    std::vector<Internal::MaterialInfo> materials =
        std::span(scene->mMaterials, scene->mNumMaterials) |
        construct<Internal::MaterialInfo> | to<std::vector>;

    std::vector<Material::Material> real_material;

    for (const auto &materialInfo : materials) {
        auto material = [&] {
            if (materialInfo.diffuse_texture_path.empty())
                return mesh_manager.m_material_manager_map
                    .allocate_material<&Material::textured_material_tag>(
                        "../../Images/image_test.png");

            return mesh_manager.m_material_manager_map
                .allocate_material<&Material::textured_material_tag>(
                    "../../Models/Sponza/" +
                    materialInfo.diffuse_texture_path.string());
        }();
        real_material.push_back(material);
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
