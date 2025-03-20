#include "VulkanWrapper/Image/Image.h"

#include "VulkanWrapper/Memory/Allocator.h"

namespace vw {
Image::Image(vk::Image image, uint32_t width, uint32_t height,
             vk::Format format, vk::ImageUsageFlags usage, Allocator *allocator,
             VmaAllocation allocation)
    : ObjectWithHandle<vk::Image>(image)
    , m_width{width}
    , m_height{height}
    , m_format{format}
    , m_usage{usage}
    , m_allocator{allocator}
    , m_allocation{allocation} {}

Image::~Image() {
    if (m_allocator != nullptr)
        vmaDestroyImage(m_allocator->handle(), handle(), m_allocation);
}

vk::Format Image::format() const noexcept { return m_format; }

} // namespace vw
