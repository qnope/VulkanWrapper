#pragma once
#include "VulkanWrapper/3rd_party.h"

namespace vw::Model {

class MeshManager;

void import_model(const std::filesystem::path &path, MeshManager &mesh_manager);

} // namespace vw::Model
