#include "VulkanWrapper/Image/ImageView.h"

#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

ImageView::ImageView(const std::shared_ptr<const Image> &image,
                     vk::UniqueImageView imageView,
                     vk::ImageSubresourceRange subresource_range)
    : ObjectWithUniqueHandle<vk::UniqueImageView>(std::move(imageView))
    , m_image{image}
    , m_subresource_range{subresource_range} {}

std::shared_ptr<const Image> ImageView::image() const noexcept {
    return m_image;
}

vk::ImageSubresourceRange ImageView::subresource_range() const noexcept {
    return m_subresource_range;
}

ImageViewBuilder::ImageViewBuilder(std::shared_ptr<const Device> device,
                                   std::shared_ptr<const Image> image)
    : m_device{std::move(device)}
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

    auto view = check_vk(m_device->handle().createImageViewUnique(info),
                         "Failed to create image view");
    return std::make_shared<const ImageView>(m_image, std::move(view),
                                             m_subResourceRange);
}

} // namespace vw
