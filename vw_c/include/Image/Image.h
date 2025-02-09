#pragma once

namespace vw {
class Image;
}

extern "C" {
void vw_destroy_image(vw::Image *image);
}
