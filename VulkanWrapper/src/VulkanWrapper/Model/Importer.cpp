#include "VulkanWrapper/Model/Importer.h"

#include "VulkanWrapper/Model/Internal/Material.h"
#include "VulkanWrapper/Model/Internal/Mesh.h"
#include "VulkanWrapper/Utils/Algos.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace vw::Model {
Importer::Importer(const std::filesystem::path &path) {
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

    std::cout << "lol" << std::endl;
}
} // namespace vw::Model
