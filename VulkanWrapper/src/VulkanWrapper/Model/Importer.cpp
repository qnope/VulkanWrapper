#include "VulkanWrapper/Model/Importer.h"

#include "VulkanWrapper/Model/Internal/MeshInfo.h"
#include "VulkanWrapper/Model/Material/BindlessMaterialManager.h"
#include "VulkanWrapper/Model/Material/Material.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Utils/Error.h"
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
        throw AssimpException(importer.GetErrorString(), path);
    }

    std::vector<Internal::MeshInfo> meshes =
        std::span(scene->mMeshes, scene->mNumMeshes) |
        construct<Internal::MeshInfo> | to<std::vector>;

    std::vector<Material::Material> real_material;

    for (const auto *mat : std::span(scene->mMaterials, scene->mNumMaterials)) {
        auto material = mesh_manager.material_manager().create_material(
            mat, directory_path);
        real_material.push_back(material);
    }

    for (const auto &mesh : meshes) {
        mesh_manager.add_mesh(mesh.full_vertices, mesh.indices,
                              real_material[mesh.material_index]);
    }
}
} // namespace vw::Model
