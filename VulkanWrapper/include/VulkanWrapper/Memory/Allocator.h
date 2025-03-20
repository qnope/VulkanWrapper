#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include <vk_mem_alloc.h>

namespace vw {
struct AllocatorImpl;

class Allocator : public ObjectWithHandle<VmaAllocator> {
  public:
    Allocator(VmaAllocator allocator);
    Allocator(const Allocator &) = delete;
    Allocator(Allocator &&) noexcept = default;

    Allocator &operator=(Allocator &&) noexcept = default;
    Allocator &operator=(const Allocator &) noexcept = delete;

    template <typename T, bool HostVisible = false>
    Buffer<T, HostVisible, VertexBufferUsage>
    allocate_vertex_buffer(VkDeviceSize size) {
        return Buffer<T, HostVisible, VertexBufferUsage>{
            allocate_buffer(size * sizeof(T), HostVisible,
                            vk::BufferUsageFlags(VertexBufferUsage),
                            vk::SharingMode::eExclusive)};
    }

    IndexBuffer allocate_index_buffer(VkDeviceSize size);

    template <typename T, bool HostVisible, VkBufferUsageFlags Usage>
    Buffer<T, HostVisible, Usage> create_buffer(VkDeviceSize number_elements) {
        return {allocate_buffer(number_elements * sizeof(T), HostVisible,
                                vk::BufferUsageFlags(Usage),
                                vk::SharingMode::eExclusive)};
    }

    [[nodiscard]] std::shared_ptr<Image>
    create_image_2D(uint32_t width, uint32_t height, bool mipmap,
                    vk::Format format, vk::ImageUsageFlags usage);

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
    const Instance *m_instance;
    const Device *m_device;
};

} // namespace vw
