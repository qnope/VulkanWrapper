#pragma once

#include "VulkanWrapper/Utils/exceptions.h"

namespace vw::Model {

using ModelNotFoundException = TaggedException<struct ModelNotFoundTag>;

class Importer {
  public:
    Importer(const std::filesystem::path &path);

  private:
};
} // namespace vw::Model
