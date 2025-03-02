#pragma once

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"

namespace vw {

class StagingBufferManager {
  public:
    StagingBufferManager(Device &device, Allocator &allocator);

    vk::CommandBuffer fill_command_buffer();

    template <typename T, VkBufferUsageFlags Usage, bool HostVisible>
    void fill_buffer(std::span<const T> data,
                     Buffer<T, HostVisible, Usage> &buffer,
                     VkDeviceSize offset_dst_buffer) {
        static_assert((Usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) ==
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        const auto size = data.size() * sizeof(T);
        m_staging_buffer.generic_copy(data.data(), data.size() * sizeof(T),
                                      m_offset_staging_buffer);

        auto function = [this, &buffer, size, offset_dst_buffer,
                         offset_src = m_offset_staging_buffer](
                            vk::CommandBuffer command_buffer) {
            const auto region = vk::BufferCopy()
                                    .setSrcOffset(offset_src)
                                    .setDstOffset(offset_dst_buffer)
                                    .setSize(size);
            command_buffer.copyBuffer(m_staging_buffer.handle(),
                                      buffer.handle(), region);
        };

        m_transfer_functions.push_back(std::move(function));

        m_offset_staging_buffer += data.size() * sizeof(T);
    }

    template <typename T, VkBufferUsageFlagBits _Usage,
              bool HostVisible = false>
    auto create_buffer(std::span<const T> data,
                       VkDeviceSize max_number_elements) {
        constexpr auto Usage = _Usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        auto buffer = m_allocator.create_buffer<T, HostVisible, Usage>(
            max_number_elements);

        fill_buffer(data, buffer, 0);
    }

  private:
    void perform_transfer(const void *data, BufferBase &buffer_base);

  private:
    Device &m_device;
    Allocator &m_allocator;
    CommandPool m_command_pool;
    Buffer<std::byte, true, VK_BUFFER_USAGE_TRANSFER_SRC_BIT> m_staging_buffer;
    VkDeviceSize m_offset_staging_buffer = 0;

    std::vector<std::function<void(vk::CommandBuffer)>> m_transfer_functions;
};
} // namespace vw
