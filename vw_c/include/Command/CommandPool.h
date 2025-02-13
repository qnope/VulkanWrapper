#pragma once

#include "vulkan/vulkan.h"

namespace vw {
class CommandPool;
class Device;
} // namespace vw

extern "C" {
struct ArrayCommandBuffer {
    VkCommandBuffer *commandBuffers;
    int size;
};

vw::CommandPool *vw_create_command_pool(const vw::Device *device);

ArrayCommandBuffer vw_command_pool_allocate(vw::CommandPool *commandPool,
                                            int number);

void vw_destroy_command_pool(vw::CommandPool *commandPool);
}
