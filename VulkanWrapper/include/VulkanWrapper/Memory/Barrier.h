#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"

namespace vw {

void execute_image_barrier_undefined_to_transfer_dst(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image);

void execute_image_barrier_undefined_to_general(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image);

void execute_image_barrier_transfer_dst_to_src(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image,
    MipLevel mip_level);

void execute_image_barrier_transfer_src_to_dst(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image);

void execute_image_barrier_transfer_dst_to_sampled(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image);

void execute_image_barrier_general_to_sampled(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image,
    vk::PipelineStageFlags2 srcStage);

void execute_image_transition(vk::CommandBuffer cmd_buffer,
                              const std::shared_ptr<const Image> &image,
                              vk::ImageLayout oldLayout,
                              vk::ImageLayout newLayout);

} // namespace vw
