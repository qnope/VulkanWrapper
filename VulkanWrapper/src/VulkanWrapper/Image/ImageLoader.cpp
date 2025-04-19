#include "VulkanWrapper/Image/ImageLoader.h"

#include "VulkanWrapper/Utils/Algos.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

template <auto f>
using static_function = std::integral_constant<decltype(f), f>;

namespace vw {
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
} // namespace vw
