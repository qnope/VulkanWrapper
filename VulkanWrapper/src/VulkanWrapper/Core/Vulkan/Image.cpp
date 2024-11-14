#include "VulkanWrapper/Core/Vulkan/Image.h"

namespace vw {
Image::Image(vk::Image image, vk::Format format)
    : ObjectWithUniqueHandle<vk::UniqueImage, vk::Image>{image}
    , m_format{format} {}

Image::Image(vk::UniqueImage image, vk::Format format)
    : vw::ObjectWithUniqueHandle<vk::UniqueImage, vk::Image>{std::move(image)}
    , m_format{format} {}

vk::Format Image::format() const noexcept { return m_format; }

} // namespace vw
