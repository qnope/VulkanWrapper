#include "VulkanWrapper/Image/CombinedImage.h"

#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Sampler.h"

namespace vw {

CombinedImage::CombinedImage(std::shared_ptr<const Image> image,
                             std::shared_ptr<const ImageView> image_view,
                             std::shared_ptr<const Sampler> sampler)
    : m_image{std::move(image)}
    , m_image_view{std::move(image_view)}
    , m_sampler{std::move(sampler)} {}

vk::Image CombinedImage::image() const noexcept { return m_image->handle(); }

vk::ImageView CombinedImage::image_view() const noexcept {
    return m_image_view->handle();
}

vk::Sampler CombinedImage::sampler() const noexcept {
    return m_sampler->handle();
}

} // namespace vw
