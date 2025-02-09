#include "Vulkan/Swapchain.h"

#include <VulkanWrapper/Vulkan/Swapchain.h>

int vw_get_swapchain_width(const vw::Swapchain *swapchain) {
    return swapchain->width();
}

int vw_get_swapchain_height(const vw::Swapchain *swapchain) {
    return swapchain->height();
}

VkFormat vw_get_swapchain_format(const vw::Swapchain *swapchain) {
    return static_cast<VkFormat>(swapchain->format());
}

void vw_destroy_swapchain(vw::Swapchain *swapchain) { delete swapchain; }

vw_SwapchainImageArray vw_swapchain_get_images(const vw::Swapchain *swapchain) {
    auto images = swapchain->images();
    vw_SwapchainImageArray array;
    array.size = images.size();
    array.images =
        static_cast<vw::Image **>(malloc(sizeof(vw::Image *) * images.size()));

    for (int i = 0; i < images.size(); ++i) {
        array.images[i] = new vw::Image{std::move(images[i])};
    }

    return array;
}

uint64_t vw_swapchain_acquire_next_image(const vw::Swapchain *swapchain,
                                         const vw::Semaphore *semaphore) {
    return swapchain->acquire_next_image(*semaphore);
}
