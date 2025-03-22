#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

using ImageViewCreationException = TaggedException<struct ImageViewCreationTag>;

class ImageView : public ObjectWithUniqueHandle<vk::UniqueImageView> {
  public:
    ImageView(const std::shared_ptr<const Image> &image,
              vk::UniqueImageView imageView);

  private:
    std::shared_ptr<const Image> m_image;
};

class ImageViewBuilder {
  public:
    ImageViewBuilder(const Device &device, std::shared_ptr<const Image> image);

    ImageViewBuilder &&setImageType(vk::ImageViewType imageViewType) &&;

    std::shared_ptr<const ImageView> build() &&;

  private:
    const Device *m_device;
    std::shared_ptr<const Image> m_image;

    vk::ImageViewType m_type = vk::ImageViewType::e2D;

    vk::ImageSubresourceRange m_subResourceRange;

    vk::ComponentMapping m_componentMapping =
        vk::ComponentMapping()
            .setA(vk::ComponentSwizzle::eIdentity)
            .setR(vk::ComponentSwizzle::eIdentity)
            .setG(vk::ComponentSwizzle::eIdentity)
            .setB(vk::ComponentSwizzle::eIdentity);
};

} // namespace vw
