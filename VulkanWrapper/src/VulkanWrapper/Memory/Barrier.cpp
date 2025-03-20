#include "VulkanWrapper/Memory/Barrier.h"

namespace vw {
void executeMemoryBarrier(vk::CommandBuffer cmd_buffer,
                          const vk::ImageMemoryBarrier2 &barrier) {
    const auto dependency =
        vk::DependencyInfo().setImageMemoryBarriers(barrier);
    cmd_buffer.pipelineBarrier2(dependency);
}

void executeImageMemoryBarrierUndefinedToTransferDst(
    vk::CommandBuffer cmd_buffer, vk::Image image,
    vk::ImageSubresourceRange range) {

    const auto img_barrier =
        vk::ImageMemoryBarrier2()
            .setSubresourceRange(range)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
            .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setImage(image);

    const auto dependency_info =
        vk::DependencyInfo().setImageMemoryBarriers(img_barrier);

    cmd_buffer.pipelineBarrier2(dependency_info);
}

void executeImageMemoryBarrierTransferToSampled(
    vk::CommandBuffer cmd_buffer, vk::Image image,
    vk::ImageSubresourceRange range) {

    const auto img_barrier =
        vk::ImageMemoryBarrier2()
            .setSubresourceRange(range)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eReadOnlyOptimal)
            .setImage(image);

    const auto dependency_info =
        vk::DependencyInfo().setImageMemoryBarriers(img_barrier);

    cmd_buffer.pipelineBarrier2(dependency_info);
}

} // namespace vw
