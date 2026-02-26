module;
#include "VulkanWrapper/3rd_party.h"

export module vw:image;
import "VulkanWrapper/vw_vulkan.h";
import :core;
import :utils;
import :vulkan;

export namespace vw {

// Forward declaration
class Allocator;

// ---- Image ----

class Image : public ObjectWithHandle<vk::Image> {
  public:
    Image(vk::Image image, Width width, Height height, Depth depth,
          MipLevel mip_level, vk::Format format, vk::ImageUsageFlags usage,
          std::shared_ptr<const Allocator> allocator, VmaAllocation allocation);

    Image(const Image &) = delete;
    Image(Image &&) = default;

    Image &operator=(const Image &) = delete;
    Image &operator=(Image &&) = delete;

    ~Image();

    [[nodiscard]] MipLevel mip_levels() const noexcept;

    [[nodiscard]] vk::Format format() const noexcept;

    [[nodiscard]] vk::ImageSubresourceRange full_range() const noexcept;

    [[nodiscard]] vk::ImageSubresourceRange
    mip_level_range(MipLevel mip_level) const noexcept;

    [[nodiscard]] vk::ImageSubresourceLayers
    mip_level_layer(MipLevel mip_level) const noexcept;

    [[nodiscard]] vk::Extent2D extent2D() const noexcept;
    [[nodiscard]] vk::Extent3D extent3D() const noexcept;

    [[nodiscard]] vk::Extent3D mip_level_extent3D(MipLevel mip_level) const;

    [[nodiscard]] std::array<vk::Offset3D, 2>
    mip_level_offsets(MipLevel mip_level) const noexcept;

  private:
    Width m_width;
    Height m_height;
    Depth m_depth;
    MipLevel m_mip_levels;
    vk::Format m_format;
    vk::ImageUsageFlags m_usage;
    std::shared_ptr<const Allocator> m_allocator;
    VmaAllocation m_allocation;
};

// ---- ImageView ----

class ImageView : public ObjectWithUniqueHandle<vk::UniqueImageView> {
  public:
    ImageView(const std::shared_ptr<const Image> &image,
              vk::UniqueImageView imageView,
              vk::ImageSubresourceRange subresource_range);

    std::shared_ptr<const Image> image() const noexcept;

    vk::ImageSubresourceRange subresource_range() const noexcept;

  private:
    std::shared_ptr<const Image> m_image;
    vk::ImageSubresourceRange m_subresource_range;
};

class ImageViewBuilder {
  public:
    ImageViewBuilder(std::shared_ptr<const Device> device,
                     std::shared_ptr<const Image> image);

    ImageViewBuilder &setImageType(vk::ImageViewType imageViewType);

    std::shared_ptr<const ImageView> build();

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Image> m_image;

    vk::ImageViewType m_type = vk::ImageViewType::e2D;

    vk::ImageSubresourceRange m_subResourceRange;

    vk::ComponentMapping m_componentMapping =
        vk::ComponentMapping()
            .setA(vk::ComponentSwizzle::eIdentity)
            .setR(vk::ComponentSwizzle::eIdentity)
            .setG(vk::ComponentSwizzle::eIdentity)
            .setB(vk::ComponentSwizzle::eIdentity);
};

// ---- Sampler ----

class Sampler : public ObjectWithUniqueHandle<vk::UniqueSampler> {
  public:
    using ObjectWithUniqueHandle<vk::UniqueSampler>::ObjectWithHandle;

  private:
};

class SamplerBuilder {
  public:
    SamplerBuilder(std::shared_ptr<const Device> device);

    std::shared_ptr<const Sampler> build();

  private:
    std::shared_ptr<const Device> m_device;
    vk::SamplerCreateInfo m_info;
};

// ---- CombinedImage ----

class CombinedImage {
  public:
    CombinedImage(std::shared_ptr<const ImageView> image_view,
                  std::shared_ptr<const Sampler> sampler);

    [[nodiscard]] vk::Image image() const noexcept;
    [[nodiscard]] vk::ImageView image_view() const noexcept;
    [[nodiscard]] std::shared_ptr<const ImageView>
    image_view_ptr() const noexcept;
    [[nodiscard]] vk::Sampler sampler() const noexcept;
    [[nodiscard]] vk::ImageSubresourceRange subresource_range() const noexcept;

  private:
    std::shared_ptr<const Image> m_image;
    std::shared_ptr<const ImageView> m_image_view;
    std::shared_ptr<const Sampler> m_sampler;
};

// ---- Mipmap ----

void generate_mipmap(vk::CommandBuffer cmd_buffer,
                     const std::shared_ptr<const Image> &image);

// ---- ImageLoader ----

struct ImageDescription {
    Width width;
    Height height;
    std::vector<std::byte> pixels;
};

ImageDescription load_image(const std::filesystem::path &path);

void save_image(const std::filesystem::path &path, Width width, Height height,
                std::span<const std::byte> pixels);

} // namespace vw
