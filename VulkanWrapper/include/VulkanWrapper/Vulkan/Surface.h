#pragma once

#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
class Surface : public ObjectWithUniqueHandle<vk::UniqueSurfaceKHR> {
  public:
    Surface(vk::UniqueSurfaceKHR surface) noexcept;
};
} // namespace vw
