#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include <vk_mem_alloc.h>

namespace vw {

class Image : public ObjectWithHandle<vk::Image> {
  public:
    Image(vk::Image image, uint32_t width, uint32_t height, uint32_t depth,
          uint32_t mip_level, vk::Format format, vk::ImageUsageFlags usage,
          Allocator *allocator, VmaAllocation allocation);

    Image(const Image &) = delete;
    Image(Image &&) = default;

    Image &operator=(const Image &) = delete;
    Image &operator=(Image &&) = delete;

    ~Image();

    [[nodiscard]] vk::Format format() const noexcept;

    [[nodiscard]] vk::ImageSubresourceRange full_range() const noexcept;

    [[nodiscard]] vk::ImageSubresourceRange
    mip_level_range(uint32_t mip_level) const noexcept;

    [[nodiscard]] vk::ImageSubresourceLayers
    mip_level_layer(uint32_t mip_level) const noexcept;

  private:
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_depth;
    uint32_t m_mip_levels;
    vk::Format m_format;
    vk::ImageUsageFlags m_usage;
    Allocator *m_allocator;
    VmaAllocation m_allocation;
};

} // namespace vw
