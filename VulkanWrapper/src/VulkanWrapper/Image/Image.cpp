#include "VulkanWrapper/Image/Image.h"

namespace vw {
Image::Image(vk::Image image, vk::Format format)
    : ObjectWithHandle<vk::Image>(image)
    , m_format{format} {}

vk::Format Image::format() const noexcept { return m_format; }

} // namespace vw
