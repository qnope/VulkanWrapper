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
    Allocator(const Device &device, VmaAllocator allocator);
    Allocator(const Allocator &) = delete;
    Allocator(Allocator &&) noexcept = default;

    Allocator &operator=(Allocator &&) noexcept = default;
    Allocator &operator=(const Allocator &) noexcept = delete;

    template <typename T, bool HostVisible = false>
    Buffer<T, HostVisible, VertexBufferUsage>
    allocate_vertex_buffer(VkDeviceSize size) const {
        return Buffer<T, HostVisible, VertexBufferUsage>{
            allocate_buffer(size * sizeof(T), HostVisible,
                            vk::BufferUsageFlags(VertexBufferUsage),
                            vk::SharingMode::eExclusive)};
    }

    [[nodiscard]] IndexBuffer allocate_index_buffer(VkDeviceSize size) const;

    template <typename T, bool HostVisible, VkBufferUsageFlags2 Usage>
    [[nodiscard]] Buffer<T, HostVisible, Usage>
    create_buffer(vk::DeviceSize number_elements) const {
        return {allocate_buffer(number_elements * sizeof(T), HostVisible,
                                vk::BufferUsageFlags(Usage),
                                vk::SharingMode::eExclusive)};
    }

    template <typename BufferType>
    [[nodiscard]] BufferType
    create_buffer(vk::DeviceSize number_elements) const {
        return create_buffer<typename BufferType::type,
                             BufferType::host_visible,
                             VkBufferUsageFlags(BufferType::flags)>(
            number_elements);
    }

    [[nodiscard]] std::shared_ptr<const Image>
    create_image_2D(Width width, Height height, bool mipmap, vk::Format format,
                    vk::ImageUsageFlags usage) const;

    ~Allocator();

  private:
    [[nodiscard]] BufferBase
    allocate_buffer(VkDeviceSize size, bool host_visible,
                    vk::BufferUsageFlags usage,
                    vk::SharingMode sharing_mode) const;

  private:
    const Device *m_device;
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
