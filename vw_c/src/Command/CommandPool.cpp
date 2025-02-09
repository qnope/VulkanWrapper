#include "Command/CommandPool.h"

#include <VulkanWrapper/Command/CommandPool.h>

vw::CommandPool *vw_create_command_pool(const vw::Device *device) {
    return new vw::CommandPool{vw::CommandPoolBuilder{*device}.build()};
}

void vw_destroy_command_pool(vw::CommandPool *commandPool) {
    delete commandPool;
}
