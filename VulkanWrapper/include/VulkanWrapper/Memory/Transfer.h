#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include <filesystem>
#include <memory>
#include <optional>

namespace vw {

/**
 * Transfer resource for managing image and buffer copy operations
 * with automatic barrier management via embedded ResourceTracker.
 */
class Transfer {
  public:
    /**
     * Creates a Transfer object with its own resource tracker.
     */
    Transfer() = default;

    /**
     * Blits from source image to destination image with automatic barrier management.
     * @param cmd Command buffer to record commands into
     * @param src Source image
     * @param dst Destination image
     * @param srcSubresource Source subresource range (default: derived from source image)
     * @param dstSubresource Destination subresource range (default: derived from destination image)
     * @param filter Filter to use for the blit operation
     */
    void blit(vk::CommandBuffer cmd,
              const std::shared_ptr<const Image> &src,
              const std::shared_ptr<const Image> &dst,
              std::optional<vk::ImageSubresourceRange> srcSubresource = std::nullopt,
              std::optional<vk::ImageSubresourceRange> dstSubresource = std::nullopt,
              vk::Filter filter = vk::Filter::eLinear);

    /**
     * Copies from source buffer to destination buffer with automatic barrier management.
     * @param cmd Command buffer to record commands into
     * @param src Source buffer handle
     * @param dst Destination buffer handle
     * @param srcOffset Offset in source buffer
     * @param dstOffset Offset in destination buffer
     * @param size Size of the copy operation
     */
    void copyBuffer(vk::CommandBuffer cmd,
                    vk::Buffer src, vk::Buffer dst,
                    vk::DeviceSize srcOffset, vk::DeviceSize dstOffset,
                    vk::DeviceSize size);

    /**
     * Copies from buffer to image with automatic barrier management.
     * @param cmd Command buffer to record commands into
     * @param src Source buffer handle
     * @param dst Destination image
     * @param srcOffset Offset in source buffer
     * @param dstSubresource Destination subresource range (default: derived from destination image)
     */
    void copyBufferToImage(vk::CommandBuffer cmd,
                           vk::Buffer src,
                           const std::shared_ptr<const Image> &dst,
                           vk::DeviceSize srcOffset,
                           std::optional<vk::ImageSubresourceRange> dstSubresource = std::nullopt);

    /**
     * Copies from image to buffer with automatic barrier management.
     * @param cmd Command buffer to record commands into
     * @param src Source image
     * @param dst Destination buffer handle
     * @param dstOffset Offset in destination buffer
     * @param srcSubresource Source subresource range (default: derived from source image)
     */
    void copyImageToBuffer(vk::CommandBuffer cmd,
                           const std::shared_ptr<const Image> &src,
                           vk::Buffer dst,
                           vk::DeviceSize dstOffset,
                           std::optional<vk::ImageSubresourceRange> srcSubresource = std::nullopt);

    /**
     * Returns the resource tracker for manual barrier management if needed.
     */
    [[nodiscard]] Barrier::ResourceTracker &resourceTracker() {
        return m_resourceTracker;
    }

    /**
     * Saves a GPU image to disk.
     * This is a high-level utility that handles:
     * - Creating a staging buffer
     * - Recording copy commands
     * - Transitioning image to final layout
     * - Submitting and waiting for completion
     * - Converting color formats (BGRA to RGBA if needed)
     * - Writing the image file
     *
     * The ResourceTracker must already have the image tracked with its current state.
     *
     * @param cmd Command buffer to record commands into (must be in recording state)
     * @param allocator Allocator for creating the staging buffer
     * @param queue Queue to submit commands to (command buffer will be ended, submitted and waited on)
     * @param image Source image to save
     * @param path File path to save to (format determined by extension: .png, .bmp, .jpg)
     * @param finalLayout Layout to transition the image to after copying (default: ePresentSrcKHR for swapchain images)
     */
    void saveToFile(
        vk::CommandBuffer cmd,
        const Allocator &allocator,
        Queue &queue,
        const std::shared_ptr<const Image> &image,
        const std::filesystem::path &path,
        vk::ImageLayout finalLayout = vk::ImageLayout::ePresentSrcKHR);

  private:
    Barrier::ResourceTracker m_resourceTracker;

    // Helper to get default subresource range for an image
    vk::ImageSubresourceRange getFullSubresourceRange(
        const std::shared_ptr<const Image> &image) const;
};

} // namespace vw
