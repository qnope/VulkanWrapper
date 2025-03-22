#include "VulkanWrapper/Image/Image.h"

#include "VulkanWrapper/Memory/Allocator.h"

namespace vw {

namespace {
vk::ImageAspectFlags aspect_flags_from_format(vk::Format format) {
    return vk::ImageAspectFlagBits::eColor;
}
} // namespace

Image::Image(vk::Image image, uint32_t width, uint32_t height, uint32_t depth,
             uint32_t mip_level, vk::Format format, vk::ImageUsageFlags usage,
             Allocator *allocator, VmaAllocation allocation)
    : ObjectWithHandle<vk::Image>(image)
    , m_width{width}
    , m_height{height}
    , m_depth{depth}
    , m_mip_levels{mip_level}
    , m_format{format}
    , m_usage{usage}
    , m_allocator{allocator}
    , m_allocation{allocation} {}

Image::~Image() {
    if (m_allocator != nullptr)
        vmaDestroyImage(m_allocator->handle(), handle(), m_allocation);
}

vk::Format Image::format() const noexcept { return m_format; }

vk::ImageSubresourceRange
Image::mip_level_range(uint32_t mip_level) const noexcept {
    return vk::ImageSubresourceRange()
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setBaseMipLevel(mip_level)
        .setLevelCount(1)
        .setAspectMask(aspect_flags_from_format(m_format));
}

vk::ImageSubresourceLayers
Image::mip_level_layer(uint32_t mip_level) const noexcept {
    return vk::ImageSubresourceLayers()
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setMipLevel(mip_level)
        .setAspectMask(aspect_flags_from_format(m_format));
}

vk::ImageSubresourceRange Image::full_range() const noexcept {
    return vk::ImageSubresourceRange()
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setBaseMipLevel(0)
        .setLevelCount(m_mip_levels)
        .setAspectMask(aspect_flags_from_format(m_format));
}

} // namespace vw
