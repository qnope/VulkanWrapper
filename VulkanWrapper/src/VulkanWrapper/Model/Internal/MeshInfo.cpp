module vw.model;
import std3rd;
import vulkan3rd;
import glm3rd;
import assimp3rd;
import vw.utils;
import vw.vulkan;
import vw.memory;
import vw.descriptors;
import vw.pipeline;

namespace vw::Model::Internal {

MeshInfo::MeshInfo(const aiMesh *mesh)
    : material_index{mesh->mMaterialIndex} {
    auto to_vec3 = [](auto x) { return glm::vec3{x.x, x.y, x.z}; };
    auto to_vec2 = [](auto x) { return glm::vec2{x.x, x.y}; };

    auto vert_span = std::span(mesh->mVertices, mesh->mNumVertices);
    auto norm_span = std::span(mesh->mNormals, mesh->mNumVertices);
    auto tan_span = std::span(mesh->mTangents, mesh->mNumVertices);
    auto bitan_span = std::span(mesh->mBitangents, mesh->mNumVertices);
    auto uv_span = std::span(mesh->mTextureCoords[0], mesh->mNumVertices);

    auto positions = std::ranges::views::transform(vert_span, to_vec3);
    auto normals = std::ranges::views::transform(norm_span, to_vec3);
    auto tangeants = std::ranges::views::transform(tan_span, to_vec3);
    auto bitangeants = std::ranges::views::transform(bitan_span, to_vec3);
    auto uvs = std::ranges::views::transform(uv_span, to_vec2);

    auto zipped = std::ranges::views::zip(positions, normals, tangeants,
                                          bitangeants, uvs);
    for (auto &&[pos, norm, tan, bitan, uv] : zipped) {
        full_vertices.push_back(FullVertex3D{pos, norm, tan, bitan, uv});
    }
    for (auto &&pos : positions) {
        vertices.push_back(Vertex3D{pos});
    }

    auto faces = std::span(mesh->mFaces, mesh->mNumFaces);
    for (const auto &face : faces) {
        for (auto index : std::span(face.mIndices, face.mNumIndices))
            indices.push_back(index);
    }
}

} // namespace vw::Model::Internal
