module;
#include "VulkanWrapper/3rd_party.h"
#include <vulkan/vulkan.hpp>
module vw;

namespace vw {
Surface::Surface(vk::UniqueSurfaceKHR surface) noexcept
    : ObjectWithUniqueHandle<vk::UniqueSurfaceKHR>{std::move(surface)} {}
} // namespace vw
