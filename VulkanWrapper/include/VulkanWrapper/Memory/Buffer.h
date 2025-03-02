#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include <vk_mem_alloc.h>

namespace vw {
class BufferBase : public ObjectWithHandle<vk::Buffer> {
  public:
    BufferBase(Allocator &allocator, vk::Buffer buffer,
               VmaAllocation allocation);

    BufferBase(const BufferBase &) = delete;
    BufferBase &operator=(const BufferBase &) = delete;

    BufferBase(BufferBase &&other) noexcept;
    BufferBase &operator=(BufferBase &&other) noexcept;

    ~BufferBase();

  protected:
    void generic_copy(const void *data, uint64_t size);

  private:
    Allocator &m_allocator;
    VmaAllocation m_allocation;
};

template <typename T, bool HostVisible, VkBufferUsageFlags flags>
class Buffer : public BufferBase {
  public:
    Buffer(BufferBase &&bufferBase)
        : BufferBase(std::move(bufferBase)) {}

    void copy(std::span<const T> span)
        requires(HostVisible)
    {
        generic_copy(span.data(), span.size() * sizeof(T));
    }
};

} // namespace vw
