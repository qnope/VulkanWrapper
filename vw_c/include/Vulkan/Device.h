#pragma once

#include <vulkan/vulkan.h>

namespace vw {
class Device;
class DeviceFinder;
class Surface;
class Queue;
class PresentQueue;
} // namespace vw

enum class VwQueueFlagBits {
    Graphics = VK_QUEUE_GRAPHICS_BIT,
    Compute = VK_QUEUE_COMPUTE_BIT,
    Transfer = VK_QUEUE_TRANSFER_BIT
};

extern "C" {
struct VwDeviceCreateArguments {
    vw::DeviceFinder *finder;
    VwQueueFlagBits queue_flags;
    const vw::Surface *surface_to_present;
};

vw::Device *vw_create_device(const VwDeviceCreateArguments *arguments);

const vw::Queue *vw_device_graphics_queue(const vw::Device *device);
const vw::PresentQueue *vw_device_present_queue(const vw::Device *device);

void vw_device_wait_idle(const vw::Device *device);

void vw_destroy_device(vw::Device *device);
}
