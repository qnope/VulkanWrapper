#pragma once

#include "enums.h"
#include <vulkan/vulkan.h>

namespace vw {
class Fence;
class Queue;
class Semaphore;
} // namespace vw

extern "C" {
struct VwQueueSubmitArguments {
    const VkCommandBuffer *command_buffers;
    unsigned command_buffer_count;
    const VwPipelineStageFlagBits *wait_stages;
    unsigned wait_stage_count;
    const VkSemaphore *wait_semaphores;
    unsigned wait_semaphore_count;
    const VkSemaphore *signal_semaphores;
    unsigned signal_semaphores_count;
};

vw::Fence *vw_queue_submit(vw::Queue *queue,
                           const VwQueueSubmitArguments *arguments);
}
