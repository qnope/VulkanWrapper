#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include <vk_mem_alloc.h>

namespace vw {

class Image : public ObjectWithHandle<vk::Image> {
  public:
    Image(vk::Image image, Width width, Height height, Depth depth,
          MipLevel mip_level, vk::Format format, vk::ImageUsageFlags usage,
          const Allocator *allocator, VmaAllocation allocation);

    Image(const Image &) = delete;
    Image(Image &&) = default;

    Image &operator=(const Image &) = delete;
    Image &operator=(Image &&) = delete;

    ~Image();

    [[nodiscard]] MipLevel mip_levels() const noexcept;

    [[nodiscard]] vk::Format format() const noexcept;

    [[nodiscard]] vk::ImageSubresourceRange full_range() const noexcept;

    [[nodiscard]] vk::ImageSubresourceRange
    mip_level_range(MipLevel mip_level) const noexcept;

    [[nodiscard]] vk::ImageSubresourceLayers
    mip_level_layer(MipLevel mip_level) const noexcept;

    [[nodiscard]] vk::Extent2D extent2D() const noexcept;
    [[nodiscard]] vk::Extent3D extent3D() const noexcept;

    [[nodiscard]] vk::Extent3D
    mip_level_extent3D(MipLevel mip_level) const noexcept;

    [[nodiscard]] std::array<vk::Offset3D, 2>
    mip_level_offsets(MipLevel mip_level) const noexcept;

  private:
    Width m_width;
    Height m_height;
    Depth m_depth;
    MipLevel m_mip_levels;
    vk::Format m_format;
    vk::ImageUsageFlags m_usage;
    const Allocator *m_allocator;
    VmaAllocation m_allocation;
};

} // namespace vw
