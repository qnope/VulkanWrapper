#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"

namespace vw {
template <typename T, bool HostVisible, VkBufferUsageFlags flags>
class BufferList {
  public:
    struct BufferInfo {
        Buffer<T, HostVisible, flags> *buffer;
        std::size_t offset;
    };

    BufferList(const Allocator &allocator) noexcept
        : m_allocator{&allocator} {}

    BufferInfo create_buffer(std::size_t size) {
        constexpr std::size_t buffer_size = 1 << 24;
        auto has_enough_place = [size](auto &buffer) {
            return buffer->buffer.size() >= size + buffer->offset;
        };
        auto it = std::ranges::find_if(m_buffer_list, has_enough_place);
        if (it != m_buffer_list.end()) {
            BufferInfo result{&it->get()->buffer, it->get()->offset};
            it->get()->offset += size;
            return result;
        }
        auto buffer = create_buffer<T, HostVisible, flags>(
            *m_allocator, std::max(buffer_size, size));
        auto &[bufferRef, offset] = *m_buffer_list.emplace_back(
            std::make_unique<BufferAndOffset>(std::move(buffer), size));
        return {&bufferRef, 0};
    }

  private:
    struct BufferAndOffset {
        Buffer<T, HostVisible, flags> buffer;
        std::size_t offset;
    };

    const Allocator *m_allocator;
    std::vector<std::unique_ptr<BufferAndOffset>> m_buffer_list;
};

using IndexBufferList = BufferList<uint32_t, false, IndexBufferUsage>;

} // namespace vw
