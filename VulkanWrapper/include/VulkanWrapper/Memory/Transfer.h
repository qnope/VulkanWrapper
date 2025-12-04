#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
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
     * Returns the resource tracker for manual barrier management if needed.
     */
    [[nodiscard]] Barrier::ResourceTracker &resourceTracker() {
        return m_resourceTracker;
    }

  private:
    Barrier::ResourceTracker m_resourceTracker;

    // Helper to get default subresource range for an image
    vk::ImageSubresourceRange getFullSubresourceRange(
        const std::shared_ptr<const Image> &image) const;
};

} // namespace vw
