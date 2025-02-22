#pragma once

#include "../Vulkan/enums.h"
#include <vulkan/vulkan.h>

namespace vw {
class Image;
class Swapchain;
class Semaphore;
} // namespace vw

extern "C" {

struct VwSwapchainImageArray {
    vw::Image **images;
    int size;
};

int vw_get_swapchain_width(const vw::Swapchain *swapchain);
int vw_get_swapchain_height(const vw::Swapchain *swapchain);
VwFormat vw_get_swapchain_format(const vw::Swapchain *swapchain);
uint64_t vw_swapchain_acquire_next_image(const vw::Swapchain *swapchain,
                                         const vw::Semaphore *semaphore);
VwSwapchainImageArray vw_swapchain_get_images(const vw::Swapchain *swapchain);

void vw_destroy_swapchain(vw::Swapchain *swapchain);
}
