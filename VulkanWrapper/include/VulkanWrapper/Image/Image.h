#pragma once

#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

class Image : public ObjectWithUniqueHandle<vk::UniqueImage, vk::Image> {
  public:
    Image(vk::Image image, vk::Format format);
    Image(vk::UniqueImage image, vk::Format format);

    vk::Format format() const noexcept;

  private:
    vk::Format m_format;
};

} // namespace vw
