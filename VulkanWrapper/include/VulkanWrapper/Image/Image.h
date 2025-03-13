#pragma once

#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

class Image : public ObjectWithHandle<vk::Image> {
  public:
    Image(vk::Image image, vk::Format format);

    vk::Format format() const noexcept;

  private:
    vk::Format m_format;
};

} // namespace vw
