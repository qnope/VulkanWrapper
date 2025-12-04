#include "VulkanWrapper/Image/ImageLoader.h"

#include "VulkanWrapper/Utils/Algos.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

template <auto f>
using static_function = std::integral_constant<decltype(f), f>;

namespace vw {

using ImageSaveException = TaggedException<struct ImageSaveTag>;

ImageDescription load_image(const std::filesystem::path &path) {
    std::unique_ptr<SDL_Surface, static_function<SDL_DestroySurface>> img{
        IMG_Load(path.string().c_str())};

    if (!img) {
        throw ImageNotFoundException{std::source_location::current()};
    }

    SDL_PixelFormat format = SDL_PIXELFORMAT_ABGR8888;

    std::unique_ptr<SDL_Surface, static_function<SDL_DestroySurface>> surface{
        SDL_ConvertSurface(img.get(), format)};

    return {.width = Width{surface->w},
            .height = Height{surface->h},
            .pixels = std::span(static_cast<const std::byte *>(surface->pixels),
                                static_cast<std::size_t>(surface->pitch *
                                                         surface->h)) |
                      to<std::vector>};
}

void save_image(const std::filesystem::path &path, Width width, Height height,
                std::span<const std::byte> pixels) {
    // Create an SDL surface from the pixel data
    // SDL expects ABGR8888 format (same as we use for loading)
    SDL_PixelFormat format = SDL_PIXELFORMAT_ABGR8888;
    int w = static_cast<int>(width);
    int h = static_cast<int>(height);
    int pitch = w * 4; // 4 bytes per pixel for RGBA8

    std::unique_ptr<SDL_Surface, static_function<SDL_DestroySurface>> surface{
        SDL_CreateSurfaceFrom(w, h, format,
                              const_cast<void *>(static_cast<const void *>(pixels.data())),
                              pitch)};

    if (!surface) {
        throw ImageSaveException{std::source_location::current()};
    }

    // Determine the format from the file extension and save
    std::string ext = path.extension().string();
    std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return std::tolower(c); });

    bool success = false;
    if (ext == ".png") {
        success = IMG_SavePNG(surface.get(), path.string().c_str());
    } else if (ext == ".bmp") {
        success = SDL_SaveBMP(surface.get(), path.string().c_str());
    } else if (ext == ".jpg" || ext == ".jpeg") {
        success = IMG_SaveJPG(surface.get(), path.string().c_str(), 90);
    } else {
        // Default to PNG for unknown extensions
        success = IMG_SavePNG(surface.get(), path.string().c_str());
    }

    if (!success) {
        throw ImageSaveException{std::source_location::current()};
    }
}

} // namespace vw
