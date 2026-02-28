export module vw.vulkan:image_loader;
import std3rd;
import vulkan3rd;

export namespace vw {

struct ImageDescription {
    Width width;
    Height height;
    std::vector<std::byte> pixels;
};

ImageDescription load_image(const std::filesystem::path &path);

void save_image(const std::filesystem::path &path, Width width, Height height,
                std::span<const std::byte> pixels);

} // namespace vw
