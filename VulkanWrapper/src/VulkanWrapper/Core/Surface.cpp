module vw.vulkan;
import std3rd;
import vulkan3rd;

namespace vw {
Surface::Surface(vk::UniqueSurfaceKHR surface) noexcept
    : ObjectWithUniqueHandle<vk::UniqueSurfaceKHR>{std::move(surface)} {}
} // namespace vw
