#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Memory/Barrier.h"

namespace vw {
void generate_mipmap(vk::CommandBuffer cmd_buffer,
                     const std::shared_ptr<const Image> &image) {
    const auto mip_levels = uint32_t(image->mip_levels());

    for (auto i = 0U; i < mip_levels - 1; ++i) {
        const auto srcLevel = MipLevel(i);
        const auto dstLevel = MipLevel(i + 1);
        const auto srcOffsets = image->mip_level_offsets(srcLevel);
        const auto dstOffsets = image->mip_level_offsets(dstLevel);
        const auto blit =
            vk::ImageBlit()
                .setSrcOffsets(srcOffsets)
                .setSrcSubresource(image->mip_level_layer(srcLevel))
                .setDstOffsets(dstOffsets)
                .setDstSubresource(image->mip_level_layer(dstLevel));
        cmd_buffer.blitImage(
            image->handle(), vk::ImageLayout::eTransferSrcOptimal,
            image->handle(), vk::ImageLayout::eTransferDstOptimal, blit,
            vk::Filter::eLinear);
        execute_image_barrier_transfer_dst_to_src(cmd_buffer, image, dstLevel);
    }
}

} // namespace vw
