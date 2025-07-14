#include "VulkanWrapper/Memory/Barrier.h"

#include "VulkanWrapper/Image/Image.h"

namespace vw {
void executeMemoryBarrier(vk::CommandBuffer cmd_buffer,
                          const vk::ImageMemoryBarrier2 &barrier) {
    const auto dependency =
        vk::DependencyInfo().setImageMemoryBarriers(barrier);
    cmd_buffer.pipelineBarrier2(dependency);
}

void execute_image_barrier_undefined_to_transfer_dst(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image) {

    const auto range = image->full_range();
    const auto img_barrier =
        vk::ImageMemoryBarrier2()
            .setSubresourceRange(range)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
            .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setImage(image->handle());

    const auto dependency_info =
        vk::DependencyInfo().setImageMemoryBarriers(img_barrier);

    cmd_buffer.pipelineBarrier2(dependency_info);
}

void execute_image_barrier_transfer_dst_to_sampled(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image) {
    const auto range = image->full_range();
    const auto img_barrier =
        vk::ImageMemoryBarrier2()
            .setSubresourceRange(range)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eReadOnlyOptimal)
            .setImage(image->handle());

    const auto dependency_info =
        vk::DependencyInfo().setImageMemoryBarriers(img_barrier);

    cmd_buffer.pipelineBarrier2(dependency_info);
}

void execute_image_barrier_transfer_src_to_dst(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image) {
    const auto range = image->full_range();
    const auto img_barrier =
        vk::ImageMemoryBarrier2()
            .setSubresourceRange(range)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferRead)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
            .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eReadOnlyOptimal)
            .setImage(image->handle());

    const auto dependency_info =
        vk::DependencyInfo().setImageMemoryBarriers(img_barrier);

    cmd_buffer.pipelineBarrier2(dependency_info);
}

void execute_image_barrier_transfer_dst_to_src(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image,
    MipLevel mip_level) {
    const auto range = image->mip_level_range(mip_level);
    const auto img_barrier =
        vk::ImageMemoryBarrier2()
            .setSubresourceRange(range)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setDstAccessMask(vk::AccessFlagBits2::eTransferRead)
            .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setImage(image->handle());

    const auto dependency_info =
        vk::DependencyInfo().setImageMemoryBarriers(img_barrier);

    cmd_buffer.pipelineBarrier2(dependency_info);
}

void execute_image_barrier_general_to_sampled(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image,
    vk::PipelineStageFlags2 srcStage) {
    const auto range = image->mip_level_range(MipLevel(0));
    const auto img_barrier =
        vk::ImageMemoryBarrier2()
            .setSubresourceRange(range)
            .setImage(image->handle())
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderStorageWrite)
            .setSrcStageMask(srcStage)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    const auto dependency_info =
        vk::DependencyInfo().setImageMemoryBarriers(img_barrier);
    cmd_buffer.pipelineBarrier2(dependency_info);
}

} // namespace vw
