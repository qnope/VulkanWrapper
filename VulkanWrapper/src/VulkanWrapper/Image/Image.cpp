#include "VulkanWrapper/Image/Image.h"

namespace vw {
Image::Image(vk::Image image, vk::Format format)
    : m_object{std::make_shared<ObjectWithHandle<vk::Image>>(image)}
    , m_format{format} {}

vk::Image Image::handle() const noexcept { return m_object->handle(); }

vk::Format Image::format() const noexcept { return m_format; }

} // namespace vw
