#include "VulkanWrapper/Memory/Transfer.h"
#include "VulkanWrapper/Image/Image.h"
#ifdef VW_HAS_SDL3_IMAGE
#include "VulkanWrapper/Image/ImageLoader.h"
#endif
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Vulkan/Queue.h"

namespace vw {

void Transfer::blit(vk::CommandBuffer cmd,
                    const std::shared_ptr<const Image> &src,
                    const std::shared_ptr<const Image> &dst,
                    std::optional<vk::ImageSubresourceRange> srcSubresourceOpt,
                    std::optional<vk::ImageSubresourceRange> dstSubresourceOpt,
                    vk::Filter filter) {

    // Use full range if not specified
    vk::ImageSubresourceRange srcSubresource = srcSubresourceOpt.value_or(src->full_range());
    vk::ImageSubresourceRange dstSubresource = dstSubresourceOpt.value_or(dst->full_range());

    // Request source image in transfer src layout
    Barrier::ImageState srcState{
        .image = src->handle(),
        .subresourceRange = srcSubresource,
        .layout = vk::ImageLayout::eTransferSrcOptimal,
        .stage = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferRead
    };
    m_resourceTracker.request(srcState);

    // Request destination image in transfer dst layout
    Barrier::ImageState dstState{
        .image = dst->handle(),
        .subresourceRange = dstSubresource,
        .layout = vk::ImageLayout::eTransferDstOptimal,
        .stage = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferWrite
    };
    m_resourceTracker.request(dstState);

    // Flush barriers before blit
    m_resourceTracker.flush(cmd);

    // Perform the blit
    vk::ImageBlit blitRegion;
    blitRegion.srcSubresource.aspectMask = srcSubresource.aspectMask;
    blitRegion.srcSubresource.mipLevel = srcSubresource.baseMipLevel;
    blitRegion.srcSubresource.baseArrayLayer = srcSubresource.baseArrayLayer;
    blitRegion.srcSubresource.layerCount = srcSubresource.layerCount;
    blitRegion.srcOffsets[0] = vk::Offset3D{0, 0, 0};
    blitRegion.srcOffsets[1] = vk::Offset3D{
        static_cast<int32_t>(src->extent3D().width),
        static_cast<int32_t>(src->extent3D().height),
        static_cast<int32_t>(src->extent3D().depth)
    };

    blitRegion.dstSubresource.aspectMask = dstSubresource.aspectMask;
    blitRegion.dstSubresource.mipLevel = dstSubresource.baseMipLevel;
    blitRegion.dstSubresource.baseArrayLayer = dstSubresource.baseArrayLayer;
    blitRegion.dstSubresource.layerCount = dstSubresource.layerCount;
    blitRegion.dstOffsets[0] = vk::Offset3D{0, 0, 0};
    blitRegion.dstOffsets[1] = vk::Offset3D{
        static_cast<int32_t>(dst->extent3D().width),
        static_cast<int32_t>(dst->extent3D().height),
        static_cast<int32_t>(dst->extent3D().depth)
    };

    cmd.blitImage(
        src->handle(), vk::ImageLayout::eTransferSrcOptimal,
        dst->handle(), vk::ImageLayout::eTransferDstOptimal,
        blitRegion, filter
    );
}

void Transfer::copyBuffer(vk::CommandBuffer cmd,
                          vk::Buffer src, vk::Buffer dst,
                          vk::DeviceSize srcOffset, vk::DeviceSize dstOffset,
                          vk::DeviceSize size) {
    
    // Request source buffer
    Barrier::BufferState srcState{
        .buffer = src,
        .offset = srcOffset,
        .size = size,
        .stage = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferRead
    };
    m_resourceTracker.request(srcState);

    // Request destination buffer
    Barrier::BufferState dstState{
        .buffer = dst,
        .offset = dstOffset,
        .size = size,
        .stage = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferWrite
    };
    m_resourceTracker.request(dstState);

    // Flush barriers
    m_resourceTracker.flush(cmd);

    // Perform the copy
    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;

    cmd.copyBuffer(src, dst, copyRegion);
}

void Transfer::copyBufferToImage(vk::CommandBuffer cmd,
                                 vk::Buffer src,
                                 const std::shared_ptr<const Image> &dst,
                                 vk::DeviceSize srcOffset,
                                 std::optional<vk::ImageSubresourceRange> dstSubresourceOpt) {

    // Use full range if not specified
    vk::ImageSubresourceRange dstSubresource = dstSubresourceOpt.value_or(dst->full_range());

    // Calculate buffer size needed (simplified - assumes tightly packed)
    vk::DeviceSize bufferSize = dst->extent3D().width * 
                                dst->extent3D().height * 
                                dst->extent3D().depth * 4; // Assume 4 bytes per pixel

    // Request source buffer
    Barrier::BufferState srcState{
        .buffer = src,
        .offset = srcOffset,
        .size = bufferSize,
        .stage = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferRead
    };
    m_resourceTracker.request(srcState);

    // Request destination image
    Barrier::ImageState dstState{
        .image = dst->handle(),
        .subresourceRange = dstSubresource,
        .layout = vk::ImageLayout::eTransferDstOptimal,
        .stage = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferWrite
    };
    m_resourceTracker.request(dstState);

    // Flush barriers
    m_resourceTracker.flush(cmd);

    // Perform the copy
    vk::BufferImageCopy copyRegion;
    copyRegion.bufferOffset = srcOffset;
    copyRegion.bufferRowLength = 0; // Tightly packed
    copyRegion.bufferImageHeight = 0; // Tightly packed
    copyRegion.imageSubresource.aspectMask = dstSubresource.aspectMask;
    copyRegion.imageSubresource.mipLevel = dstSubresource.baseMipLevel;
    copyRegion.imageSubresource.baseArrayLayer = dstSubresource.baseArrayLayer;
    copyRegion.imageSubresource.layerCount = dstSubresource.layerCount;
    copyRegion.imageOffset = vk::Offset3D{0, 0, 0};
    copyRegion.imageExtent = dst->extent3D();

    cmd.copyBufferToImage(
        src, dst->handle(), vk::ImageLayout::eTransferDstOptimal, copyRegion
    );
}

void Transfer::copyImageToBuffer(vk::CommandBuffer cmd,
                                 const std::shared_ptr<const Image> &src,
                                 vk::Buffer dst,
                                 vk::DeviceSize dstOffset,
                                 std::optional<vk::ImageSubresourceRange> srcSubresourceOpt) {

    // Use full range if not specified
    vk::ImageSubresourceRange srcSubresource = srcSubresourceOpt.value_or(src->full_range());

    // Calculate buffer size needed (simplified - assumes tightly packed)
    vk::DeviceSize bufferSize = src->extent3D().width *
                                src->extent3D().height *
                                src->extent3D().depth * 4; // Assume 4 bytes per pixel

    // Request source image in transfer src layout
    Barrier::ImageState srcState{
        .image = src->handle(),
        .subresourceRange = srcSubresource,
        .layout = vk::ImageLayout::eTransferSrcOptimal,
        .stage = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferRead
    };
    m_resourceTracker.request(srcState);

    // Request destination buffer
    Barrier::BufferState dstState{
        .buffer = dst,
        .offset = dstOffset,
        .size = bufferSize,
        .stage = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferWrite
    };
    m_resourceTracker.request(dstState);

    // Flush barriers
    m_resourceTracker.flush(cmd);

    // Perform the copy
    vk::BufferImageCopy copyRegion;
    copyRegion.bufferOffset = dstOffset;
    copyRegion.bufferRowLength = 0; // Tightly packed
    copyRegion.bufferImageHeight = 0; // Tightly packed
    copyRegion.imageSubresource.aspectMask = srcSubresource.aspectMask;
    copyRegion.imageSubresource.mipLevel = srcSubresource.baseMipLevel;
    copyRegion.imageSubresource.baseArrayLayer = srcSubresource.baseArrayLayer;
    copyRegion.imageSubresource.layerCount = srcSubresource.layerCount;
    copyRegion.imageOffset = vk::Offset3D{0, 0, 0};
    copyRegion.imageExtent = src->extent3D();

    cmd.copyImageToBuffer(
        src->handle(), vk::ImageLayout::eTransferSrcOptimal, dst, copyRegion
    );
}

vk::ImageSubresourceRange Transfer::getFullSubresourceRange(
    const std::shared_ptr<const Image> &image) const {
    return image->full_range();
}

#ifdef VW_HAS_SDL3_IMAGE
void Transfer::saveToFile(
    vk::CommandBuffer cmd,
    const Allocator &allocator,
    Queue &queue,
    const std::shared_ptr<const Image> &image,
    const std::filesystem::path &path,
    vk::ImageLayout finalLayout) {

    // Calculate buffer size (assumes 4 bytes per pixel for RGBA/BGRA formats)
    const auto extent = image->extent3D();
    const size_t bufferSize = extent.width * extent.height * extent.depth * 4;

    // Create staging buffer
    using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
    auto stagingBuffer = create_buffer<StagingBuffer>(allocator, bufferSize);

    // Copy image to buffer (ResourceTracker already knows image state)
    copyImageToBuffer(cmd, image, stagingBuffer.handle(), 0);

    // Transition image to final layout
    m_resourceTracker.request(Barrier::ImageState{
        .image = image->handle(),
        .subresourceRange = image->full_range(),
        .layout = finalLayout,
        .stage = vk::PipelineStageFlagBits2::eBottomOfPipe,
        .access = vk::AccessFlagBits2::eNone});

    m_resourceTracker.flush(cmd);

    std::ignore = cmd.end();

    queue.enqueue_command_buffer(cmd);
    queue.submit({}, {}, {}).wait();

    // Get pixel data
    auto pixels = stagingBuffer.as_vector(0, bufferSize);

    // Check if we need to convert BGRA to RGBA
    // Common BGRA formats used by swapchains
    vk::Format format = image->format();
    bool isBGRA = (format == vk::Format::eB8G8R8A8Unorm ||
                   format == vk::Format::eB8G8R8A8Srgb ||
                   format == vk::Format::eB8G8R8A8Snorm);

    if (isBGRA) {
        // Swap B and R channels
        for (size_t i = 0; i < pixels.size(); i += 4) {
            std::swap(pixels[i], pixels[i + 2]);
        }
    }

    // Save to file
    save_image(path, Width(extent.width), Height(extent.height),
               std::span<const std::byte>(pixels));
}
#endif

} // namespace vw
