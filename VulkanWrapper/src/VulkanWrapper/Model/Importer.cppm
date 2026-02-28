export module vw.model:importer;
import std3rd;

export namespace vw::Model {

class MeshManager;

void import_model(const std::filesystem::path &path, MeshManager &mesh_manager);

} // namespace vw::Model
