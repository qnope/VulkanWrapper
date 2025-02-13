#include "Command/CommandPool.h"

#include <VulkanWrapper/Command/CommandPool.h>

vw::CommandPool *vw_create_command_pool(const vw::Device *device) {
    return new vw::CommandPool{vw::CommandPoolBuilder{*device}.build()};
}

void vw_destroy_command_pool(vw::CommandPool *commandPool) {
    delete commandPool;
}

ArrayCommandBuffer vw_command_pool_allocate(vw::CommandPool *commandPool,
                                            int number) {
    auto result = commandPool->allocate(number);
    ArrayCommandBuffer array{
        .commandBuffers = static_cast<VkCommandBuffer *>(
            malloc(sizeof(VkCommandBuffer) * result.size())),
        .size = static_cast<int>(result.size())};
    std::ranges::copy(result, array.commandBuffers);
    return array;
}
