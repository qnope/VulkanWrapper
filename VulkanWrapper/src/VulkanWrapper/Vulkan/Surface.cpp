module;
#include "VulkanWrapper/3rd_party.h"
module vw;

import "VulkanWrapper/vw_vulkan.h";
namespace vw {
Surface::Surface(vk::UniqueSurfaceKHR surface) noexcept
    : ObjectWithUniqueHandle<vk::UniqueSurfaceKHR>{std::move(surface)} {}
} // namespace vw
