#include "VulkanWrapper/Model/Importer.h"

#include "VulkanWrapper/Model/Internal/MaterialInfo.h"
#include "VulkanWrapper/Model/Internal/MeshInfo.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialManager.h"
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

    const auto directory_path = path.parent_path();
    const auto *scene = importer.ReadFile(path.string().c_str(), post_process);

    if (scene == nullptr) {
        throw ModelNotFoundException{std::source_location::current()};
    }

    std::vector<Internal::MeshInfo> meshes =
        std::span(scene->mMeshes, scene->mNumMeshes) |
        construct<Internal::MeshInfo> | to<std::vector>;

    std::vector<Internal::MaterialInfo> materials =
        std::span(scene->mMaterials, scene->mNumMaterials) |
        std::views::transform([&](auto *material) {
            return Internal::MaterialInfo(material, directory_path);
        }) |
        to<std::vector>;

    std::vector<Material::Material> real_material;

    for (const auto &materialInfo : materials) {
        auto material =
            mesh_manager.m_material_factory.allocate_material(materialInfo);
        real_material.push_back(material);
    }

    for (const auto &mesh : meshes) {
        auto [full_vertex_buffer, vertex_offset] =
            mesh_manager.m_full_vertex_buffer.create_buffer(
                mesh.full_vertices.size());
        auto [index_buffer, first_index] =
            mesh_manager.m_index_buffer.create_buffer(mesh.indices.size());

        auto vertex_buffer = mesh_manager.m_vertex_buffer
                                 .create_buffer(mesh.full_vertices.size())
                                 .buffer;

        mesh_manager.m_meshes.emplace_back(
            vertex_buffer, full_vertex_buffer, index_buffer,
            real_material[mesh.material_index], mesh.indices.size(),
            vertex_offset, first_index, mesh.vertices.size());

        mesh_manager.m_staging_buffer_manager.fill_buffer<Vertex3D>(
            mesh.vertices, *vertex_buffer, vertex_offset);

        mesh_manager.m_staging_buffer_manager.fill_buffer<FullVertex3D>(
            mesh.full_vertices, *full_vertex_buffer, vertex_offset);

        mesh_manager.m_staging_buffer_manager.fill_buffer<uint32_t>(
            mesh.indices, *index_buffer, first_index);
    }
}
} // namespace vw::Model
