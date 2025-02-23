#pragma once

#include "../Synchronization/Semaphore.h"
#include "../Vulkan/Swapchain.h"
#include "vulkan/vulkan.h"

namespace vw {
class PresentQueue;
}

extern "C" {

struct VwPresentQueueArguments {
    const vw::Swapchain *swapchain;
    int image_index;
    const vw::Semaphore *wait_semaphore;
};

void vw_present_queue_present(const vw::PresentQueue *present_queue,
                              const VwPresentQueueArguments *arguments);
}
