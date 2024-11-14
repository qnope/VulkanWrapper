#pragma once

#include "ObjectWithHandle.h"
#include "VulkanWrapper/Core/fwd.h"

namespace vw {

class ImageView : public ObjectWithUniqueHandle<vk::UniqueImageView> {
    friend class ImageViewBuilder;

  private:
    ImageView(const Image &image, vk::UniqueImageView imageView);

  private:
    const Image &m_image;
};

class ImageViewBuilder {
  public:
    ImageViewBuilder(Device &device, const Image &image);

    ImageViewBuilder setImageType(vk::ImageViewType imageViewType) &&;

    ImageView build() &&;

  private:
    Device &m_device;
    const Image &m_image;

    vk::ImageViewType m_type = vk::ImageViewType::e2D;

    vk::ImageSubresourceRange m_subResourceRange =
        vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setBaseMipLevel(0)
            .setLevelCount(1);

    vk::ComponentMapping m_componentMapping =
        vk::ComponentMapping()
            .setA(vk::ComponentSwizzle::eIdentity)
            .setR(vk::ComponentSwizzle::eIdentity)
            .setG(vk::ComponentSwizzle::eIdentity)
            .setB(vk::ComponentSwizzle::eIdentity);
};

} // namespace vw
