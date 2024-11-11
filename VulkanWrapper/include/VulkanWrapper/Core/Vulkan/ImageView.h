#pragma once

#include "ObjectWithHandle.h"

namespace vw {

class ImageView  : public ObjectWithUniqueHandle<vk::UniqueImageView>
{
    friend class ImageViewBuilder;
private:
    ImageView(vk::UniqueImageView imageView);

};

class ImageViewBuilder {

};

}
