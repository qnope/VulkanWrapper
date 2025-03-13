#include "Image/ImageView.h"

#include <VulkanWrapper/Image/ImageView.h>

vw::ImageView *
vw_create_image_view(const VwImageViewCreateArguments *arguments) {
    auto imageView = vw::ImageViewBuilder(*arguments->device, *arguments->image)
                         .setImageType(vk::ImageViewType(arguments->image_type))
                         .build();
    return new vw::ImageView{std::move(*imageView)};
}

void vw_destroy_image_view(vw::ImageView *image_view) { delete image_view; }
