#pragma once

#include "VulkanWrapper/Image/CombinedImage.h"

namespace vw::Model {
struct Material {
    CombinedImage combined_image;
    vk::DescriptorSet descriptor_set;
};
} // namespace vw::Model
