#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include <vk_mem_alloc.h>

namespace vw {
struct AllocatorImpl;

class Allocator : public ObjectWithHandle<VmaAllocator> {
  public:
    Allocator(VmaAllocator allocator);

    template <typename T, bool HostVisible>
    Buffer<T, HostVisible> allocate_vertex_buffer(uint64_t size) {
        auto usage =
            vk::BufferUsageFlags(vk::BufferUsageFlagBits::eVertexBuffer);
        if constexpr (!HostVisible) {
            usage |= vk::BufferUsageFlagBits::eTransferDst;
        }
        return Buffer<T, HostVisible>{
            allocate_buffer(size * sizeof(T), HostVisible,
                            vk::BufferUsageFlagBits::eVertexBuffer,
                            vk::SharingMode::eExclusive)};
    }

    ~Allocator();

  private:
    BufferBase allocate_buffer(uint64_t size, bool host_visible,
                               vk::BufferUsageFlagBits usage,
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
