#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include <vk_mem_alloc.h>

namespace vw {

class Image : public ObjectWithHandle<vk::Image> {
  public:
    Image(vk::Image image, uint32_t width, uint32_t height, vk::Format format,
          vk::ImageUsageFlags usage, Allocator *allocator,
          VmaAllocation allocation);

    Image(const Image &) = delete;
    Image(Image &&) = default;

    Image &operator=(const Image &) = delete;
    Image &operator=(Image &&) = delete;

    ~Image();

    [[nodiscard]] vk::Format format() const noexcept;

  private:
    uint32_t m_width;
    uint32_t m_height;
    vk::Format m_format;
    vk::ImageUsageFlags m_usage;
    Allocator *m_allocator;
    VmaAllocation m_allocation;
};

} // namespace vw
