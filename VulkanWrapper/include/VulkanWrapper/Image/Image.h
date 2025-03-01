#pragma once

#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

class Image {
  public:
    Image(vk::Image image, vk::Format format);

    vk::Image handle() const noexcept;
    vk::Format format() const noexcept;

  private:
    std::shared_ptr<ObjectWithHandle<vk::Image>> m_object;
    vk::Format m_format;
};

} // namespace vw
