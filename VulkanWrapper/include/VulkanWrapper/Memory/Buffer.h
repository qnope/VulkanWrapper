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
    BufferBase(const Allocator &allocator, vk::Buffer buffer,
               VmaAllocation allocation, VkDeviceSize size);

    BufferBase(const BufferBase &) = delete;
    BufferBase &operator=(const BufferBase &) = delete;

    BufferBase(BufferBase &&other) noexcept;
    BufferBase &operator=(BufferBase &&other) noexcept;

    [[nodiscard]] VkDeviceSize size_bytes() const noexcept;

    void generic_copy(const void *data, VkDeviceSize size, VkDeviceSize offset);

    ~BufferBase();

  private:
    const Allocator *m_allocator;
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

    std::size_t size() const noexcept { return size_bytes() / sizeof(T); }
};

using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;

} // namespace vw
