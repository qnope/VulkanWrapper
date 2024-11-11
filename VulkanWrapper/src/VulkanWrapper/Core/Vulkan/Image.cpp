#include "VulkanWrapper/Core/Vulkan/Image.h"

namespace vw {
Image::Image(vk::Image image)
    : ObjectWithUniqueHandle<vk::UniqueImage, vk::Image>{image} {}

Image::Image(vk::UniqueImage image)
    : vw::ObjectWithUniqueHandle<vk::UniqueImage, vk::Image>{
          std::move(image)} {}

} // namespace vw
