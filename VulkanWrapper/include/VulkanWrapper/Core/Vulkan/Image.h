#pragma once

#include "ObjectWithHandle.h"

namespace vw {

class Image : public ObjectWithUniqueHandle<vk::UniqueImage, vk::Image> {
  public:
    Image(vk::Image image);
    Image(vk::UniqueImage image);
};

} // namespace vw
