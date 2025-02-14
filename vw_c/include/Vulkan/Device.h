#pragma once

#include <vulkan/vulkan.h>

namespace vw {
class Device;
class DeviceFinder;
class Surface;
class Queue;
class PresentQueue;
} // namespace vw

extern "C" {
vw::Device *vw_create_device(vw::DeviceFinder *finder,
                             VkQueueFlagBits queueFlags,
                             const vw::Surface *surfaceToPresent);

const vw::Queue *vw_device_graphics_queue(const vw::Device *device);
const vw::PresentQueue *vw_device_present_queue(const vw::Device *device);

void vw_device_wait_idle(const vw::Device *device);

void vw_destroy_device(vw::Device *device);
}
