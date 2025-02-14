#pragma once

#include <vulkan/vulkan.h>

namespace vw {
class Fence;
class Queue;
class Semaphore;
} // namespace vw

extern "C" {
struct VwQueueSubmitArguments {
    const VkCommandBuffer *command_buffers;
    uint32_t command_buffer_count;
    const VkPipelineStageFlags *wait_stages;
    uint32_t wait_stage_count;
    const VkSemaphore *wait_semaphores;
    uint32_t wait_semaphore_count;
    const VkSemaphore *signal_semaphores;
    uint32_t signal_semaphores_count;
    const vw::Fence *fence;
};

void vw_queue_submit(const vw::Queue *queue,
                     const VwQueueSubmitArguments *arguments);
}
