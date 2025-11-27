#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/Utils/exceptions.h"

namespace vw {

using ImageNotFoundException = TaggedException<struct ImageNotFoundTag>;

struct ImageDescription {
    Width width;
    Height height;
    std::vector<std::byte> pixels;
};

ImageDescription load_image(const std::filesystem::path &path);

} // namespace vw
