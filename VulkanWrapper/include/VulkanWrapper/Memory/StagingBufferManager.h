#pragma once

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"

namespace vw {

class StagingBuffer {
  public:
    StagingBuffer(const Allocator &allocator, vk::DeviceSize size);

    [[nodiscard]] vk::Buffer handle() const noexcept;
    [[nodiscard]] vk::DeviceSize offset() const noexcept;
    [[nodiscard]] bool handle_data(vk::DeviceSize size) const noexcept;

    template <typename T> void fill_buffer(std::span<const T> data) {
        m_buffer.generic_copy(data.data(), data.size_bytes(), m_offset);
        m_offset += data.size_bytes();
    }

    template <typename T, VkBufferUsageFlags Usage, bool HostVisible>
    [[nodiscard]] auto fill_buffer(std::span<const T> data,
                                   Buffer<T, HostVisible, Usage> &buffer,
                                   uint32_t offset_dst_buffer) {
        static_assert((Usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) ==
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        const auto size = data.size_bytes();

        auto function = [src_buffer_handle = m_buffer.handle(), &buffer, size,
                         offset_dst_buffer, offset_src = m_offset](
                            vk::CommandBuffer command_buffer) {
            const auto region = vk::BufferCopy()
                                    .setSrcOffset(offset_src)
                                    .setDstOffset(offset_dst_buffer * sizeof(T))
                                    .setSize(size);
            command_buffer.copyBuffer(src_buffer_handle, buffer.handle(),
                                      region);
        };
        fill_buffer(data);
        return function;
    }

  private:
    Buffer<std::byte, true, VK_BUFFER_USAGE_TRANSFER_SRC_BIT> m_buffer;
    vk::DeviceSize m_offset = 0;
};

class StagingBufferManager {
  public:
    StagingBufferManager(const Device &device, const Allocator &allocator);

    vk::CommandBuffer fill_command_buffer();

    template <typename T, VkBufferUsageFlags Usage, bool HostVisible>
    auto fill_buffer(std::span<const T> data,
                     Buffer<T, HostVisible, Usage> &buffer,
                     uint32_t offset_dst_buffer) {
        auto &staging_buffer = get_staging_buffer(data.size_bytes());

        auto function =
            staging_buffer.fill_buffer(data, buffer, offset_dst_buffer);
        m_transfer_functions.emplace_back(function);
    }

    CombinedImage stage_image_from_path(const std::filesystem::path &path,
                                        bool mipmaps);

  private:
    void perform_transfer(const void *data, BufferBase &buffer_base);
    StagingBuffer &get_staging_buffer(vk::DeviceSize size);

    const Device *m_device;
    const Allocator *m_allocator;
    CommandPool m_command_pool;
    std::vector<StagingBuffer> m_staging_buffers;

    std::vector<std::function<void(vk::CommandBuffer)>> m_transfer_functions;
    std::shared_ptr<const Sampler> m_sampler;
};
} // namespace vw
