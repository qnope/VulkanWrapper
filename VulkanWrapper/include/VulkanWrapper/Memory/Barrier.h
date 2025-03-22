#pragma once

#include "VulkanWrapper/fwd.h"

namespace vw {

void execute_image_barrier_undefined_to_transfer_dst(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image);

void execute_image_barrier_transfer_dst_to_sampled(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image,
    uint32_t mip_level);

} // namespace vw
