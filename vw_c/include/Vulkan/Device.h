#pragma once

#include <vulkan/vulkan.h>

namespace vw {
class Device;
class DeviceFinder;
class Surface;
} // namespace vw

extern "C" {
vw::Device *vw_create_device(vw::DeviceFinder *finder,
                             VkQueueFlagBits queueFlags,
                             const vw::Surface *surfaceToPresent);

void vw_destroy_device(vw::Device *device);
}
