#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/BufferList.h"

namespace vw {

class StagingBufferManager {
  public:
    StagingBufferManager(std::shared_ptr<const Device> device,
                         std::shared_ptr<const Allocator> allocator);

    [[nodiscard]] vk::CommandBuffer fill_command_buffer();

    template <typename T, bool HostVisible, VkBufferUsageFlags Usage>
    void fill_buffer(std::span<const T> data,
                     const Buffer<T, HostVisible, Usage> &buffer,
                     uint32_t offset_dst_buffer) {
        auto [staging_buffer, offset_src] =
            m_staging_buffers.create_buffer(data.size_bytes());

        static_assert((Usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) ==
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        staging_buffer->generic_copy(data, offset_src);

        auto function = [src_buffer_handle = staging_buffer->handle(), &buffer,
                         size = data.size_bytes(),
                         offset_dst = offset_dst_buffer * sizeof(T),
                         offset_src = offset_src * sizeof(std::byte)](
                            vk::CommandBuffer command_buffer) {
            const auto region = vk::BufferCopy()
                                    .setSrcOffset(offset_src)
                                    .setDstOffset(offset_dst)
                                    .setSize(size);
            command_buffer.copyBuffer(src_buffer_handle, buffer.handle(),
                                      region);
        };

        m_transfer_functions.emplace_back(function);
    }

    [[nodiscard]] CombinedImage
    stage_image_from_path(const std::filesystem::path &path, bool mipmaps);

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Allocator> m_allocator;
    CommandPool m_command_pool;
    BufferList<std::byte, true, VK_BUFFER_USAGE_TRANSFER_SRC_BIT>
        m_staging_buffers;

    std::vector<std::function<void(vk::CommandBuffer)>> m_transfer_functions;
    std::shared_ptr<const Sampler> m_sampler;
};
} // namespace vw
