#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include <vk_mem_alloc.h>

namespace vw {

constexpr VkBufferUsageFlags VertexBufferUsage =
    VkBufferUsageFlags{vk::BufferUsageFlagBits::eVertexBuffer |
                       vk::BufferUsageFlagBits::eTransferDst};

constexpr VkBufferUsageFlags IndexBufferUsage =
    VkBufferUsageFlags{vk::BufferUsageFlagBits::eIndexBuffer |
                       vk::BufferUsageFlagBits::eTransferDst};

constexpr VkBufferUsageFlags UniformBufferUsage =
    VkBufferUsageFlags{vk::BufferUsageFlagBits::eUniformBuffer |
                       vk::BufferUsageFlagBits::eTransferDst};

class BufferBase : public ObjectWithHandle<vk::Buffer> {
  public:
    BufferBase(Allocator &allocator, vk::Buffer buffer,
               VmaAllocation allocation, VkDeviceSize size);

    BufferBase(const BufferBase &) = delete;
    BufferBase &operator=(const BufferBase &) = delete;

    BufferBase(BufferBase &&other) noexcept;
    BufferBase &operator=(BufferBase &&other) noexcept;

    VkDeviceSize size_in_bytes() const noexcept;

    void generic_copy(const void *data, VkDeviceSize size, VkDeviceSize offset);

    ~BufferBase();

  private:
    Allocator &m_allocator;
    VmaAllocation m_allocation;
    VkDeviceSize m_size_in_bytes;
};

template <typename T, bool HostVisible, VkBufferUsageFlags flags>
class Buffer : public BufferBase {
  public:
    static consteval bool does_support(vk::BufferUsageFlags usage) {
        return (vk::BufferUsageFlags(flags) & usage) == usage;
    }

    Buffer(BufferBase &&bufferBase)
        : BufferBase(std::move(bufferBase)) {}

    void copy(std::span<const T> span, VkDeviceSize offset)
        requires(HostVisible)
    {
        generic_copy(span.data(), span.size() * sizeof(T), offset);
    }
};

using IndexBuffer = Buffer<unsigned, false, IndexBufferUsage>;

} // namespace vw
