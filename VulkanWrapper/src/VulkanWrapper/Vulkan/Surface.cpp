#include "VulkanWrapper/Vulkan/Surface.h"

namespace vw {
Surface::Surface(vk::UniqueSurfaceKHR surface) noexcept
    : ObjectWithUniqueHandle<vk::UniqueSurfaceKHR>{std::move(surface)} {}
} // namespace vw
