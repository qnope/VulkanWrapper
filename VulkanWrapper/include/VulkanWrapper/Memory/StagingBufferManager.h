#pragma once

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"

namespace vw {

class StagingBuffer {
  public:
    StagingBuffer(Allocator &allocator, vk::DeviceSize size);

    bool handle_data(vk::DeviceSize size) const noexcept;

    template <typename T, VkBufferUsageFlags Usage, bool HostVisible>
    auto fill_buffer(std::span<const T> data,
                     Buffer<T, HostVisible, Usage> &buffer,
                     VkDeviceSize offset_dst_buffer) {
        static_assert((Usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) ==
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        const auto size = data.size() * sizeof(T);
        m_buffer.generic_copy(data.data(), data.size() * sizeof(T), m_offset);

        auto function = [this, &buffer, size, offset_dst_buffer,
                         offset_src =
                             m_offset](vk::CommandBuffer command_buffer) {
            const auto region = vk::BufferCopy()
                                    .setSrcOffset(offset_src)
                                    .setDstOffset(offset_dst_buffer)
                                    .setSize(size);
            command_buffer.copyBuffer(m_buffer.handle(), buffer.handle(),
                                      region);
        };
        m_offset += data.size() * sizeof(T);
        return function;
    }

  private:
    Buffer<std::byte, true, VK_BUFFER_USAGE_TRANSFER_SRC_BIT> m_buffer;
    vk::DeviceSize m_offset = 0;
};

class StagingBufferManager {
  public:
    StagingBufferManager(Device &device, Allocator &allocator);

    vk::CommandBuffer fill_command_buffer();

    template <typename T, VkBufferUsageFlags Usage, bool HostVisible>
    auto fill_buffer(std::span<const T> data,
                     Buffer<T, HostVisible, Usage> &buffer,
                     VkDeviceSize offset_dst_buffer) {
        auto &staging_buffer = get_staging_buffer(data.size_bytes());

        auto function =
            staging_buffer.fill_buffer(data, buffer, offset_dst_buffer);
        m_transfer_functions.emplace_back(function);
    }

  private:
    void perform_transfer(const void *data, BufferBase &buffer_base);
    StagingBuffer &get_staging_buffer(vk::DeviceSize size);

    Device *m_device;
    Allocator *m_allocator;
    CommandPool m_command_pool;
    std::vector<StagingBuffer> m_staging_buffers;

    std::vector<std::function<void(vk::CommandBuffer)>> m_transfer_functions;
};
} // namespace vw
