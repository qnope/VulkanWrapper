export module vw.model:mesh_info;
import std3rd;
import assimp3rd;
import vw.descriptors;

export namespace vw::Model::Internal {

struct MeshInfo {
    MeshInfo(const aiMesh *mesh);

    std::vector<Vertex3D> vertices;
    std::vector<FullVertex3D> full_vertices;
    std::vector<uint32_t> indices;
    uint32_t material_index;
};

} // namespace vw::Model::Internal
