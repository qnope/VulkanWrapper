#include "Vulkan/Queue.h"

#include <span>
#include <type_traits>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>
#include <VulkanWrapper/Vulkan/Queue.h>

vw::Fence *vw_queue_submit(vw::Queue *queue,
                           const VwQueueSubmitArguments *arguments) {
    static_assert(std::is_standard_layout_v<vk::CommandBuffer>);
    static_assert(std::is_standard_layout_v<vk::PipelineStageFlagBits>);

    std::span command_buffers = {
        reinterpret_cast<const vk::CommandBuffer *>(arguments->command_buffers),
        arguments->command_buffer_count};

    std::span wait_stages = {reinterpret_cast<const vk::PipelineStageFlags *>(
                                 arguments->wait_stages),
                             arguments->wait_stage_count};

    std::span wait_semaphores = {
        reinterpret_cast<const vk::Semaphore *>(arguments->wait_semaphores),
        arguments->wait_semaphore_count};

    std::span signal_semaphores = {
        reinterpret_cast<const vk::Semaphore *>(arguments->signal_semaphores),
        arguments->signal_semaphores_count};

    return new vw::Fence{
        queue->submit(wait_stages, wait_semaphores, signal_semaphores)};
}
