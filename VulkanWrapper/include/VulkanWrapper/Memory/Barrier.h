#pragma once

namespace vw {

void executeImageMemoryBarrierUndefinedToTransferDst(
    vk::CommandBuffer cmd_buffer, vk::Image image,
    vk::ImageSubresourceRange range);

void executeImageMemoryBarrierTransferToSampled(
    vk::CommandBuffer cmd_buffer, vk::Image image,
    vk::ImageSubresourceRange range);

} // namespace vw
