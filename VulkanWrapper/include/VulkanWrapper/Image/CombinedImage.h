#pragma once

#include "VulkanWrapper/fwd.h"

namespace vw {
class CombinedImage {
  public:
    CombinedImage(std::shared_ptr<const Image> image,
                  std::shared_ptr<const ImageView> image_view,
                  std::shared_ptr<const Sampler> sampler);

    [[nodiscard]] vk::Image image() const noexcept;
    [[nodiscard]] vk::ImageView image_view() const noexcept;
    [[nodiscard]] vk::Sampler sampler() const noexcept;

  private:
    std::shared_ptr<const Image> m_image;
    std::shared_ptr<const ImageView> m_image_view;
    std::shared_ptr<const Sampler> m_sampler;
};
} // namespace vw
