#include "VulkanWrapper/Model/Internal/Mesh.h"

#include "VulkanWrapper/Utils/Algos.h"
#include <assimp/mesh.h>
namespace vw::Model::Internal {
Mesh::Mesh(const aiMesh *mesh) {
    material_index = mesh->mMaterialIndex;
    auto to_vec3 =
        std::views::transform([](auto x) { return glm::vec3{x.x, x.y, x.z}; });
    auto to_vec2 =
        std::views::transform([](auto x) { return glm::vec2{x.x, x.y}; });

    const auto positions =
        std::span(mesh->mVertices, mesh->mNumVertices) | to_vec3;
    const auto normals =
        std::span(mesh->mNormals, mesh->mNumVertices) | to_vec3;
    const auto tangeants =
        std::span(mesh->mTangents, mesh->mNumVertices) | to_vec3;
    const auto bitangeants =
        std::span(mesh->mBitangents, mesh->mNumVertices) | to_vec3;
    const auto uvs =
        std::span(mesh->mTextureCoords[0], mesh->mNumVertices) | to_vec2;

    vertices =
        std::views::zip(positions, normals, tangeants, bitangeants, uvs) |
        construct<FullVertex3D> | to<std::vector>;

    const auto faces = std::span(mesh->mFaces, mesh->mNumFaces);

    for (const auto &face : faces) {
        for (auto index : std::span(face.mIndices, face.mNumIndices))
            indices.push_back(index);
    }
}

} // namespace vw::Model::Internal
