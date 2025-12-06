#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include <memory>
#include <vk_mem_alloc.h>

namespace vw {

class Image;

class Allocator : public std::enable_shared_from_this<Allocator> {
    friend class AllocatorBuilder;

  public:
    Allocator(const Allocator &) = delete;
    Allocator(Allocator &&) = delete;

    Allocator &operator=(Allocator &&) = delete;
    Allocator &operator=(const Allocator &) = delete;

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
    Allocator(std::shared_ptr<const Device> device, VmaAllocator allocator);

    struct Impl {
        std::shared_ptr<const Device> device;
        VmaAllocator allocator;

        Impl(std::shared_ptr<const Device> dev, VmaAllocator alloc);
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
    AllocatorBuilder(std::shared_ptr<const Instance> instance,
                     std::shared_ptr<const Device> device);

    std::shared_ptr<Allocator> build() &&;

  private:
    std::shared_ptr<const Instance> m_instance;
    std::shared_ptr<const Device> m_device;
};

} // namespace vw
