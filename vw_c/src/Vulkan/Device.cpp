#include "Vulkan/Device.h"

#include <Vulkan/vulkan.hpp>
#include <VulkanWrapper/Vulkan/Device.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Vulkan/Instance.h>
#include <VulkanWrapper/Vulkan/Surface.h>

vw::Device *vw_create_device(const VwDeviceCreateArguments *arguments) {
    if (arguments->surface_to_present)
        std::move(*arguments->finder)
            .with_presentation(arguments->surface_to_present->handle());

    if (arguments->with_synchronization_2)
        std::move(*arguments->finder).with_synchronization_2();

    return new vw::Device{
        std::move(*arguments->finder)
            .with_queue(vk::QueueFlags(VkQueueFlags(arguments->queue_flags)))
            .build()};
}

vw::Queue *vw_device_graphics_queue(vw::Device *device) {
    return &device->graphicsQueue();
}

const vw::PresentQueue *vw_device_present_queue(const vw::Device *device) {
    return &device->presentQueue();
}

void vw_device_wait_idle(const vw::Device *device) { device->wait_idle(); }

void vw_destroy_device(vw::Device *device) { delete device; }
