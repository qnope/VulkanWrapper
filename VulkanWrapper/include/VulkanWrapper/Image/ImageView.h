#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

using ImageViewCreationException = TaggedException<struct ImageViewCreationTag>;

class ImageView {
    friend class ImageViewBuilder;

  public:
    vk::ImageView handle() const noexcept;

  private:
    ImageView(const Image &image, vk::UniqueImageView imageView);

  private:
    std::shared_ptr<ObjectWithUniqueHandle<vk::UniqueImageView>> m_object;
    Image m_image;
};

class ImageViewBuilder {
  public:
    ImageViewBuilder(const Device &device, const Image &image);

    ImageViewBuilder &&setImageType(vk::ImageViewType imageViewType) &&;

    ImageView build() &&;

  private:
    const Device &m_device;
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
