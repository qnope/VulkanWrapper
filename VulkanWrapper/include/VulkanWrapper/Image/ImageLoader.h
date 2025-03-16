#pragma once

#include "VulkanWrapper/Utils/exceptions.h"

namespace vw {

using ImageNotFoundException = TaggedException<struct ImageNotFoundTag>;

struct ImageDescription {
    int width;
    int height;
    std::vector<std::byte> pixels;
};

ImageDescription load_image(const std::filesystem::path &path);

} // namespace vw
