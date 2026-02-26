// Non-module translation unit: defines the Vulkan dynamic dispatch loader
// storage and the DefaultDispatcher() function.
//
// This is the ONLY file that must include vulkan.hpp directly (for the
// VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE macro).  All other
// translation units get Vulkan types through the vw_vulkan.h header unit.
#include "VulkanWrapper/3rd_party.h"
#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace vw {
const vk::detail::DispatchLoaderDynamic &DefaultDispatcher() {
    return VULKAN_HPP_DEFAULT_DISPATCHER;
}
} // namespace vw
