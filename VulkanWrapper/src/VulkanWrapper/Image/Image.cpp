#include "VulkanWrapper/Image/Image.h"

namespace vw {
Image::Image(vk::Image image, vk::Format format)
    : m_object{std::make_shared<
          ObjectWithUniqueHandle<vk::UniqueImage, vk::Image>>(image)}
    , m_format{format} {}

Image::Image(vk::UniqueImage image, vk::Format format)
    : m_object{std::make_shared<
          vw::ObjectWithUniqueHandle<vk::UniqueImage, vk::Image>>(
          std::move(image))}
    , m_format{format} {}

vk::Image Image::handle() const noexcept { return m_object->handle(); }

vk::Format Image::format() const noexcept { return m_format; }

} // namespace vw
