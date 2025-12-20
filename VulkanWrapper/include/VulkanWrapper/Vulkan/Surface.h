#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
class Surface : public ObjectWithUniqueHandle<vk::UniqueSurfaceKHR> {
  public:
    Surface(vk::UniqueSurfaceKHR surface) noexcept;
};
} // namespace vw
