#include "VulkanWrapper/Core/Vulkan/ImageView.h"

namespace vw {

ImageView::ImageView(vk::UniqueImageView imageView) : ObjectWithUniqueHandle<vk::UniqueImageView>{std::move(imageView)}
{}

}
