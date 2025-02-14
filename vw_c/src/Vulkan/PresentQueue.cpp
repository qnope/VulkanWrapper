#include "Vulkan/PresentQueue.h"

#include <VulkanWrapper/Vulkan/PresentQueue.h>

void vw_present_queue_present(const vw::PresentQueue *present_queue,
                              const VwPresentQueueArguments *arguments) {
    present_queue->present(*arguments->swapchain, arguments->image_index,
                           *arguments->wait_semaphore);
}
