#pragma once

namespace vw {
class CommandPool;
class Device;
} // namespace vw

extern "C" {
vw::CommandPool *vw_create_command_pool(const vw::Device *device);
void vw_destroy_command_pool(vw::CommandPool *commandPool);
}
