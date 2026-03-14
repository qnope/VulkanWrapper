#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/RenderPass/Slot.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include <map>
#include <memory>
#include <string_view>
#include <vector>

namespace vw {

namespace Barrier {
class ResourceTracker;
} // namespace Barrier

// Structure to store cached image + view
struct CachedImage {
    std::shared_ptr<const Image> image;
    std::shared_ptr<const ImageView> view;
};

class RenderPipeline;

/**
 * @brief Non-templated base class for render passes with lazy image
 * allocation
 *
 * Each pass identifies its image slots using the Slot enum.
 * Images are lazily allocated on first use and cached by
 * (slot, width, height, frame_index). When dimensions change,
 * old images with different dimensions are deleted.
 */
class RenderPass {
  public:
    RenderPass(std::shared_ptr<Device> device,
               std::shared_ptr<Allocator> allocator);

    RenderPass(const RenderPass &) = delete;
    RenderPass(RenderPass &&) = delete;
    RenderPass &operator=(RenderPass &&) = delete;
    RenderPass &operator=(const RenderPass &) = delete;

    virtual ~RenderPass() = default;

    // Introspection: which slots this pass reads/writes
    virtual std::vector<Slot> input_slots() const = 0;
    virtual std::vector<Slot> output_slots() const = 0;

    // Execute the pass
    virtual void execute(vk::CommandBuffer cmd,
                         Barrier::ResourceTracker &tracker,
                         Width width, Height height,
                         size_t frame_index) = 0;

    // Get output images after execute()
    std::vector<std::pair<Slot, CachedImage>> result_images() const;

    // Human-readable name for debugging / pipeline graph
    virtual std::string_view name() const = 0;

    // Reset any temporal accumulation (no-op by default)
    virtual void reset_accumulation() {}

    // Wire an input image for the given slot
    void set_input(Slot slot, CachedImage image);

  protected:
    /**
     * @brief Get or create an image for the given slot and dimensions
     *
     * If an image with matching (slot, width, height, frame_index)
     * exists, returns it. Otherwise, creates a new image and caches
     * it. Images with different dimensions are removed from cache
     * to avoid memory overhead.
     */
    const CachedImage &get_or_create_image(Slot slot, Width width,
                                           Height height,
                                           size_t frame_index,
                                           vk::Format format,
                                           vk::ImageUsageFlags usage);

    // Access an input image wired by RenderPipeline
    const CachedImage &get_input(Slot slot) const;

    std::shared_ptr<Device> m_device;
    std::shared_ptr<Allocator> m_allocator;

    friend class RenderPipeline;

  private:

    // Output image cache keyed by (Slot, width, height, frame_index)
    struct ImageKey {
        Slot slot;
        uint32_t width;
        uint32_t height;
        size_t frame_index;
        auto operator<=>(const ImageKey &) const = default;
    };
    std::map<ImageKey, CachedImage> m_image_cache;

    // Input images from predecessor passes
    std::map<Slot, CachedImage> m_inputs;
};

} // namespace vw
