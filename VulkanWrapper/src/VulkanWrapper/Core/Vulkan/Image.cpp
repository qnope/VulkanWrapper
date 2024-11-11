#include "VulkanWrapper/Core/Vulkan/Image.h"

namespace r3d {
Image::Image(vk::Image image)
    : ObjectWithUniqueHandle<vk::UniqueImage, vk::Image>{image} {}

Image::Image(vk::UniqueImage image)
    : r3d::ObjectWithUniqueHandle<vk::UniqueImage, vk::Image>{
          std::move(image)} {}

} // namespace r3d
