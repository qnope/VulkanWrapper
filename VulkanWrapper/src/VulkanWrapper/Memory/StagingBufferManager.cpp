#include "VulkanWrapper/Memory/StagingBufferManager.h"

#include "VulkanWrapper/Command/CommandPool.h"

namespace vw {

static CommandPool create_command_pool(Device &device) {
    return CommandPoolBuilder(device).build();
}

StagingBufferManager::StagingBufferManager(Device &device, Allocator &allocator)
    : m_device{device}
    , m_allocator{allocator}
    , m_command_pool(create_command_pool(device))
    , m_staging_buffer{
          allocator.create_buffer<std::byte, true,
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT>(1 << 22)} {}

vk::CommandBuffer StagingBufferManager::fill_command_buffer() {
    auto cmd_buffer = m_command_pool.allocate(1)[0];

    vk::CommandBufferBeginInfo info(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    std::ignore = cmd_buffer.begin(&info);
    for (auto &function : m_transfer_functions)
        function(cmd_buffer);
    std::ignore = cmd_buffer.end();

    return cmd_buffer;
}

} // namespace vw
