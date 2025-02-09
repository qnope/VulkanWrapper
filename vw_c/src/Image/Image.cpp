#include "Image/Image.h"

#include <VulkanWrapper/Image/Image.h>

void vw_destroy_image(vw::Image *image) { delete image; }
