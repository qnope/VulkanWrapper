#include "VulkanWrapper/Image/Image.h"

#include "VulkanWrapper/Memory/Allocator.h"

namespace vw {

namespace {
vk::ImageAspectFlags aspect_flags_from_format(vk::Format format) {
    switch (format) {
        using enum vk::Format;
    case eD16Unorm:
    case eD16UnormS8Uint:
    case eD24UnormS8Uint:
    case eD32Sfloat:
    case eD32SfloatS8Uint:
        return vk::ImageAspectFlagBits::eDepth;
    default:
        return vk::ImageAspectFlagBits::eColor;
    }
}
} // namespace

Image::Image(vk::Image image, Width width, Height height, Depth depth,
             MipLevel mip_level, vk::Format format, vk::ImageUsageFlags usage,
             std::optional<Allocator> allocator, VmaAllocation allocation)
    : ObjectWithHandle<vk::Image>(image)
    , m_width{width}
    , m_height{height}
    , m_depth{depth}
    , m_mip_levels{mip_level}
    , m_format{format}
    , m_usage{usage}
    , m_allocator{std::move(allocator)}
    , m_allocation{allocation} {}

Image::~Image() {
    if (m_allocator.has_value()) {
        vmaDestroyImage(m_allocator->handle(), handle(), m_allocation);
    }
}

MipLevel Image::mip_levels() const noexcept { return m_mip_levels; }

vk::Format Image::format() const noexcept { return m_format; }

vk::ImageSubresourceRange
Image::mip_level_range(MipLevel mip_level) const noexcept {
    return vk::ImageSubresourceRange()
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setBaseMipLevel(uint32_t(mip_level))
        .setLevelCount(1)
        .setAspectMask(aspect_flags_from_format(m_format));
}

vk::ImageSubresourceLayers
Image::mip_level_layer(MipLevel mip_level) const noexcept {
    return vk::ImageSubresourceLayers()
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setMipLevel(uint32_t(mip_level))
        .setAspectMask(aspect_flags_from_format(m_format));
}

vk::ImageSubresourceRange Image::full_range() const noexcept {
    return vk::ImageSubresourceRange()
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setBaseMipLevel(0)
        .setLevelCount(uint32_t(m_mip_levels))
        .setAspectMask(aspect_flags_from_format(m_format));
}

vk::Extent2D Image::extent2D() const noexcept {
    return {uint32_t(m_width), uint32_t(m_height)};
}

vk::Extent3D Image::extent3D() const noexcept {
    return {uint32_t(m_width), uint32_t(m_height), uint32_t(m_depth)};
}

vk::Extent3D Image::mip_level_extent3D(MipLevel mip_level) const noexcept {
    assert(mip_level < m_mip_levels);
    auto width = uint32_t(m_width) >> uint32_t(mip_level);
    auto height = uint32_t(m_height) >> uint32_t(mip_level);
    auto depth = uint32_t(m_depth) >> uint32_t(mip_level);

    return {std::max(1U, width), std::max(1U, height), std::max(1U, depth)};
}

std::array<vk::Offset3D, 2>
Image::mip_level_offsets(MipLevel mip_level) const noexcept {
    const auto [width, height, depth] = mip_level_extent3D(mip_level);
    return {vk::Offset3D(),
            vk::Offset3D(int32_t(width), int32_t(height), int32_t(depth))};
}

} // namespace vw
