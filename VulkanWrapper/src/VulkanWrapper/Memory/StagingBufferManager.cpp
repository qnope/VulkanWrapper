#include "VulkanWrapper/Memory/StagingBufferManager.h"

#include "VulkanWrapper/Command/CommandPool.h"

constexpr auto STAGING_BUFFER_SIZE = 1 << 22;

namespace vw {

static vk::DeviceSize compute_size(vk::DeviceSize size) {
    const auto max_size =
        double(std::max<vk::DeviceSize>(size, STAGING_BUFFER_SIZE));

    return vk::DeviceSize(std::pow(2, std::ceil(std::log2(max_size))));
}

static CommandPool create_command_pool(Device &device) {
    return CommandPoolBuilder(device).build();
}

StagingBuffer::StagingBuffer(Allocator &allocator, vk::DeviceSize size)
    : m_buffer{
          allocator.create_buffer<std::byte, true,
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT>(size)} {}

bool StagingBuffer::handle_data(vk::DeviceSize size) const noexcept {
    return (m_buffer.size_bytes() - m_offset) >= size;
}

StagingBufferManager::StagingBufferManager(Device &device, Allocator &allocator)
    : m_device{&device}
    , m_allocator{&allocator}
    , m_command_pool(create_command_pool(device)) {}

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

StagingBuffer &StagingBufferManager::get_staging_buffer(vk::DeviceSize size) {
    auto does_buffer_handle_data = [size](const StagingBuffer &buffer) {
        return buffer.handle_data(size);
    };

    auto it = std::ranges::find_if(m_staging_buffers, does_buffer_handle_data);
    if (it != m_staging_buffers.end())
        return *it;

    const auto new_size = compute_size(size);
    return m_staging_buffers.emplace_back(*m_allocator, new_size);
}

} // namespace vw
