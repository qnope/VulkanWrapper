#pragma once

#include "ObjectWithHandle.h"

namespace r3d {

class Image : public ObjectWithUniqueHandle<vk::UniqueImage, vk::Image> {
  public:
    Image(vk::Image image);
    Image(vk::UniqueImage image);
};

} // namespace r3d
