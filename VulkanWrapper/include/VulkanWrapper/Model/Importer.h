#pragma once

#include "VulkanWrapper/Utils/exceptions.h"

namespace vw::Model {

class MeshManager;

using ModelNotFoundException = TaggedException<struct ModelNotFoundTag>;

class Importer {
  public:
    Importer(const std::filesystem::path &path, MeshManager &mesh_manager);

  private:
};
} // namespace vw::Model
