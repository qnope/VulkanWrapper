#include "Vulkan/Device.h"

#include <Vulkan/vulkan.hpp>
#include <VulkanWrapper/Vulkan/Device.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Vulkan/Instance.h>
#include <VulkanWrapper/Vulkan/Surface.h>

vw::Device *vw_create_device(vw::DeviceFinder *finder,
                             VkQueueFlagBits queueFlags,
                             const vw::Surface *surfaceToPresent) {
    if (surfaceToPresent)
        return new vw::Device(std::move(*finder)
                                  .with_queue(vk::QueueFlags(queueFlags))
                                  .with_presentation(surfaceToPresent->handle())
                                  .build());
    return new vw::Device{
        std::move(*finder).with_queue(vk::QueueFlags(queueFlags)).build()};
}

const vw::Queue *vw_device_graphics_queue(const vw::Device *device) {
    return &device->graphicsQueue();
}

const vw::PresentQueue *vw_device_present_queue(const vw::Device *device) {
    return &device->presentQueue();
}

void vw_device_wait_idle(const vw::Device *device) { device->wait_idle(); }

void vw_destroy_device(vw::Device *device) { delete device; }
