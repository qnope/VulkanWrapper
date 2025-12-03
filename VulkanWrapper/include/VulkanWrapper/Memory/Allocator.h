#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include <memory>
#include <vk_mem_alloc.h>

namespace vw {

class Image;

class Allocator {
  public:
    Allocator(const Device &device, VmaAllocator allocator);
    Allocator(const Allocator &) = default;
    Allocator(Allocator &&other) noexcept = default;

    Allocator &operator=(Allocator &&other) noexcept = default;
    Allocator &operator=(const Allocator &) noexcept = default;

    [[nodiscard]] VmaAllocator handle() const noexcept;

    [[nodiscard]] IndexBuffer allocate_index_buffer(VkDeviceSize size) const;

    [[nodiscard]] std::shared_ptr<const Image>
    create_image_2D(Width width, Height height, bool mipmap, vk::Format format,
                    vk::ImageUsageFlags usage) const;

    [[nodiscard]] BufferBase
    allocate_buffer(VkDeviceSize size, bool host_visible,
                    vk::BufferUsageFlags usage,
                    vk::SharingMode sharing_mode) const;

  private:

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
