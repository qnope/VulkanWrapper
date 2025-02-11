#include "VulkanWrapper/Image/ImageView.h"

#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

ImageView::ImageView(const Image &image, vk::UniqueImageView imageView)
    : m_object{std::make_shared<ObjectWithUniqueHandle<vk::UniqueImageView>>(
          std::move(imageView))}
    , m_image{image} {}

vk::ImageView ImageView::handle() const noexcept { return m_object->handle(); }

ImageViewBuilder::ImageViewBuilder(const Device &device, const Image &image)
    : m_device{device}
    , m_image{image} {}

ImageViewBuilder &&ImageViewBuilder::setImageType(vk::ImageViewType type) && {
    m_type = type;
    return std::move(*this);
}

ImageView ImageViewBuilder::build() && {
    const auto info = vk::ImageViewCreateInfo()
                          .setImage(m_image.handle())
                          .setFormat(m_image.format())
                          .setViewType(m_type)
                          .setComponents(m_componentMapping)
                          .setSubresourceRange(m_subResourceRange);

    auto view = m_device.handle().createImageViewUnique(info);
    if (view.result != vk::Result::eSuccess)
        throw ImageViewCreationException{std::source_location::current()};
    return ImageView(m_image, std::move(view.value));
}

} // namespace vw
