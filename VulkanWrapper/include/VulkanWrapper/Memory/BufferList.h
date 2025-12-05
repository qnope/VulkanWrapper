#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"

namespace vw {
template <typename T, bool HostVisible, VkBufferUsageFlags flags>
class BufferList {
  public:
    struct BufferInfo {
        std::shared_ptr<Buffer<T, HostVisible, flags>> buffer;
        std::size_t offset;
    };

    BufferList(std::shared_ptr<const Allocator> allocator) noexcept
        : m_allocator{std::move(allocator)} {}

    BufferInfo create_buffer(std::size_t size, std::size_t alignment = 1) {
        constexpr std::size_t buffer_size = 1 << 24;

        // Helper to align offset
        auto align_up = [](std::size_t value, std::size_t align) {
            return (value + align - 1) & ~(align - 1);
        };

        auto has_enough_place = [size, alignment, align_up](auto &buffer_and_offset) {
            std::size_t aligned_offset = align_up(buffer_and_offset.offset, alignment);
            return buffer_and_offset.buffer->size() >= size + aligned_offset;
        };
        auto it = std::ranges::find_if(m_buffer_list, has_enough_place);
        if (it != m_buffer_list.end()) {
            std::size_t aligned_offset = align_up(it->offset, alignment);
            BufferInfo result{it->buffer, aligned_offset};
            it->offset = aligned_offset + size;
            return result;
        }
        auto buffer = std::make_shared<Buffer<T, HostVisible, flags>>(
            vw::create_buffer<T, HostVisible, flags>(*m_allocator, std::max(buffer_size, size)));
        auto &buffer_and_offset = m_buffer_list.emplace_back(BufferAndOffset{buffer, size});
        return {buffer_and_offset.buffer, 0};
    }

  private:
    struct BufferAndOffset {
        std::shared_ptr<Buffer<T, HostVisible, flags>> buffer;
        std::size_t offset;
    };

    std::shared_ptr<const Allocator> m_allocator;
    std::vector<BufferAndOffset> m_buffer_list;
};

using IndexBufferList = BufferList<uint32_t, false, IndexBufferUsage>;

} // namespace vw
