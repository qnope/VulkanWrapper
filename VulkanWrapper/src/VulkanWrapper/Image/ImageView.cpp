#include "VulkanWrapper/Image/ImageView.h"

#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

ImageView::ImageView(const std::shared_ptr<const Image> &image,
                     vk::UniqueImageView imageView)
    : ObjectWithUniqueHandle<vk::UniqueImageView>(std::move(imageView))
    , m_image{image} {}

ImageViewBuilder::ImageViewBuilder(const Device &device,
                                   std::shared_ptr<const Image> image)
    : m_device{&device}
    , m_image{std::move(image)}
    , m_subResourceRange{m_image->full_range()} {}

ImageViewBuilder &&ImageViewBuilder::setImageType(vk::ImageViewType type) && {
    m_type = type;
    return std::move(*this);
}

std::shared_ptr<const ImageView> ImageViewBuilder::build() && {
    const auto info = vk::ImageViewCreateInfo()
                          .setImage(m_image->handle())
                          .setFormat(m_image->format())
                          .setViewType(m_type)
                          .setComponents(m_componentMapping)
                          .setSubresourceRange(m_subResourceRange);

    auto view = m_device->handle().createImageViewUnique(info);
    if (view.result != vk::Result::eSuccess) {
        throw ImageViewCreationException{std::source_location::current()};
    }
    return std::make_shared<const ImageView>(m_image, std::move(view.value));
}

} // namespace vw
