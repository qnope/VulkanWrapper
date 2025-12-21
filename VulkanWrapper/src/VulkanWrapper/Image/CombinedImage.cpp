#include "VulkanWrapper/Image/CombinedImage.h"

#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Sampler.h"

namespace vw {

CombinedImage::CombinedImage(std::shared_ptr<const ImageView> image_view,
                             std::shared_ptr<const Sampler> sampler)
    : m_image{image_view->image()}
    , m_image_view{std::move(image_view)}
    , m_sampler{std::move(sampler)} {}

vk::Image CombinedImage::image() const noexcept { return m_image->handle(); }

vk::ImageView CombinedImage::image_view() const noexcept {
    return m_image_view->handle();
}

std::shared_ptr<const ImageView> CombinedImage::image_view_ptr() const noexcept {
    return m_image_view;
}

vk::Sampler CombinedImage::sampler() const noexcept {
    return m_sampler->handle();
}

vk::ImageSubresourceRange CombinedImage::subresource_range() const noexcept {
    return m_image_view->subresource_range();
}

} // namespace vw
