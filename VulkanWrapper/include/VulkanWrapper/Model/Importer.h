#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/Utils/exceptions.h"

namespace vw::Model {

class MeshManager;

using ModelNotFoundException = TaggedException<struct ModelNotFoundTag>;

void import_model(const std::filesystem::path &path, MeshManager &mesh_manager);

} // namespace vw::Model
