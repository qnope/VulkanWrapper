#pragma once
#include "VulkanWrapper/3rd_party.h"
#include <span>

namespace vw {

struct ImageDescription {
    Width width;
    Height height;
    std::vector<std::byte> pixels;
};

ImageDescription load_image(const std::filesystem::path &path);

/**
 * Saves pixel data to an image file on disk.
 * @param path Path where to save the image (format determined by extension:
 * .png, .bmp, .jpg)
 * @param width Width of the image in pixels
 * @param height Height of the image in pixels
 * @param pixels Pixel data in RGBA8 format (4 bytes per pixel)
 */
void save_image(const std::filesystem::path &path, Width width, Height height,
                std::span<const std::byte> pixels);

} // namespace vw
