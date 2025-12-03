#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include <memory>
#include <vk_mem_alloc.h>

namespace vw {

class Allocator {
  public:
    Allocator(const Device &device, VmaAllocator allocator);
    Allocator(const Allocator &) = default;
    Allocator(Allocator &&other) noexcept = default;

    Allocator &operator=(Allocator &&other) noexcept = default;
    Allocator &operator=(const Allocator &) noexcept = default;

    [[nodiscard]] VmaAllocator handle() const noexcept;

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

  private:
    [[nodiscard]] BufferBase
    allocate_buffer(VkDeviceSize size, bool host_visible,
                    vk::BufferUsageFlags usage,
                    vk::SharingMode sharing_mode) const;

    struct Impl {
        const Device *device;
        VmaAllocator allocator;

        Impl(const Device &dev, VmaAllocator alloc);
        ~Impl();

        Impl(const Impl &) = delete;
        Impl &operator=(const Impl &) = delete;
        Impl(Impl &&) = delete;
        Impl &operator=(Impl &&) = delete;
    };

    std::shared_ptr<Impl> m_impl;
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
