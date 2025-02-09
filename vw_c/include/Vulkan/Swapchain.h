#pragma once

#include <vulkan/vulkan.h>

namespace vw {
class Image;
class Swapchain;
class Semaphore;
} // namespace vw

extern "C" {

struct vw_SwapchainImageArray {
    vw::Image **images;
    int size;
};

int vw_get_swapchain_width(const vw::Swapchain *swapchain);
int vw_get_swapchain_height(const vw::Swapchain *swapchain);
VkFormat vw_get_swapchain_format(const vw::Swapchain *swapchain);
uint64_t vw_swapchain_acquire_next_image(const vw::Swapchain *swapchain,
                                         const vw::Semaphore *semaphore);
vw_SwapchainImageArray vw_swapchain_get_images(const vw::Swapchain *swapchain);

void vw_destroy_swapchain(vw::Swapchain *swapchain);
}
