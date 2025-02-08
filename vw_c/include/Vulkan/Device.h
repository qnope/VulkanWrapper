#pragma once

#include <Vulkan/vulkan.h>

namespace vw {
class Device;
class DeviceFinder;
class Surface;
} // namespace vw

extern "C" {
vw::Device *vw_create_device(vw::DeviceFinder *finder, VkQueueFlags queueFlags,
                             const vw::Surface *surfaceToPresent);

void vw_destroy_device(vw::Device *device);
}
