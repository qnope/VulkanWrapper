#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include <vk_mem_alloc.h>

namespace vw {
struct AllocatorImpl;

constexpr VkBufferUsageFlags VertexBufferUsage =
    VkBufferUsageFlags{vk::BufferUsageFlagBits::eVertexBuffer |
                       vk::BufferUsageFlagBits::eTransferDst};

class Allocator : public ObjectWithHandle<VmaAllocator> {
  public:
    Allocator(VmaAllocator allocator);

    template <typename T, bool HostVisible = false>
    Buffer<T, HostVisible, VertexBufferUsage>
    allocate_vertex_buffer(VkDeviceSize size) {
        return Buffer<T, HostVisible, VertexBufferUsage>{
            allocate_buffer(size * sizeof(T), HostVisible,
                            vk::BufferUsageFlags(VertexBufferUsage),
                            vk::SharingMode::eExclusive)};
    }

    template <typename T, bool HostVisible, VkBufferUsageFlags Usage>
    Buffer<T, HostVisible, Usage> create_buffer(VkDeviceSize number_elements) {
        return {allocate_buffer(number_elements * sizeof(T), HostVisible,
                                vk::BufferUsageFlags(Usage),
                                vk::SharingMode::eExclusive)};
    }

    ~Allocator();

  private:
    BufferBase allocate_buffer(VkDeviceSize size, bool host_visible,
                               vk::BufferUsageFlags usage,
                               vk::SharingMode sharing_mode);
};

class AllocatorBuilder {
  public:
    AllocatorBuilder(const Instance &instance, const Device &device);

    Allocator build() &&;

  private:
    const Instance &m_instance;
    const Device &m_device;
};
} // namespace vw
