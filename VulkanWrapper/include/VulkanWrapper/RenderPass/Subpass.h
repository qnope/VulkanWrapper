#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include <map>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace vw {

// Structure to store cached image + view
struct CachedImage {
    std::shared_ptr<const Image> image;
    std::shared_ptr<const ImageView> view;
};

/**
 * @brief Base class for render passes with lazy image allocation
 *
 * @tparam SlotEnum Enumeration defining the image slots for this pass
 *
 * Each pass defines its own SlotEnum to identify its output images.
 * Images are lazily allocated on first use and cached by (slot, width, height, frame_index).
 * When dimensions change, old images with different dimensions are deleted.
 */
template <typename SlotEnum>
class Subpass {
  public:
    Subpass(std::shared_ptr<Device> device, std::shared_ptr<Allocator> allocator)
        : m_device(std::move(device))
        , m_allocator(std::move(allocator)) {}

    Subpass(const Subpass &) = delete;
    Subpass(Subpass &&) = delete;
    Subpass &operator=(Subpass &&) = delete;
    Subpass &operator=(const Subpass &) = delete;

    virtual ~Subpass() = default;

  protected:
    /**
     * @brief Get or create an image for the given slot and dimensions
     *
     * If an image with matching (slot, width, height, frame_index) exists, returns it.
     * Otherwise, creates a new image and caches it.
     * Images with different dimensions are removed from cache to avoid memory overhead.
     *
     * @param slot The image slot identifier
     * @param width Image width
     * @param height Image height
     * @param frame_index Frame index for multi-buffering
     * @param format Vulkan format for the image
     * @param usage Vulkan usage flags for the image
     * @return Reference to the cached image structure
     */
    const CachedImage &get_or_create_image(SlotEnum slot, Width width,
                                           Height height, size_t frame_index,
                                           vk::Format format,
                                           vk::ImageUsageFlags usage) {
        ImageKey key{slot, static_cast<uint32_t>(width),
                     static_cast<uint32_t>(height), frame_index};

        // Check if image already exists
        auto it = m_image_cache.find(key);
        if (it != m_image_cache.end()) {
            return it->second;
        }

        // Remove images with different dimensions to avoid memory overhead
        std::erase_if(m_image_cache, [&](const auto &entry) {
            return entry.first.slot == slot &&
                   (entry.first.width != key.width ||
                    entry.first.height != key.height);
        });

        // Create new image
        auto image = m_allocator->create_image_2D(width, height, false, format,
                                                  usage);

        auto view = ImageViewBuilder(m_device, image)
                        .setImageType(vk::ImageViewType::e2D)
                        .build();

        auto [inserted_it, success] =
            m_image_cache.emplace(key, CachedImage{std::move(image), std::move(view)});

        return inserted_it->second;
    }

    std::shared_ptr<Device> m_device;
    std::shared_ptr<Allocator> m_allocator;

  private:
    struct ImageKey {
        SlotEnum slot;
        uint32_t width;
        uint32_t height;
        size_t frame_index;

        auto operator<=>(const ImageKey &) const = default;
    };

    std::map<ImageKey, CachedImage> m_image_cache;
};

} // namespace vw
