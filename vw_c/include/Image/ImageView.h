#pragma once

#include <vulkan/vulkan.h>

namespace vw {
class Device;
class ImageView;
class Image;
} // namespace vw

extern "C" {
struct VwImageViewCreateArguments {
    const vw::Device *device;
    const vw::Image *image;
    VkImageViewType image_type;
};

vw::ImageView *
vw_create_image_view(const VwImageViewCreateArguments *arguments);
void vw_destroy_image_view(vw::ImageView *image_view);
}
