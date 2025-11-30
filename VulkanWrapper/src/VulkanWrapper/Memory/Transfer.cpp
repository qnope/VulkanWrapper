#include "VulkanWrapper/Memory/Transfer.h"
#include "VulkanWrapper/Image/Image.h"

namespace vw {

void Transfer::blit(vk::CommandBuffer cmd,
                    const std::shared_ptr<const Image> &src,
                    const std::shared_ptr<const Image> &dst,
                    vk::ImageSubresourceRange srcSubresource,
                    vk::ImageSubresourceRange dstSubresource,
                    vk::Filter filter) {
    
    // Use full range if not specified
    if (srcSubresource.layerCount == 0) {
        srcSubresource = src->full_range();
    }
    if (dstSubresource.layerCount == 0) {
        dstSubresource = dst->full_range();
    }

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
                                 vk::ImageSubresourceRange dstSubresource) {
    
    // Use full range if not specified
    if (dstSubresource.layerCount == 0) {
        dstSubresource = dst->full_range();
    }

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

vk::ImageSubresourceRange Transfer::getFullSubresourceRange(
    const std::shared_ptr<const Image> &image) const {
    return image->full_range();
}

} // namespace vw
