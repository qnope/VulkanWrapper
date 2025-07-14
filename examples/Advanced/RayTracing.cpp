#include "RayTracing.h"

#include <VulkanWrapper/Memory/Barrier.h>

void RayTracingPass::execute(
    vk::CommandBuffer buffer,
    std::shared_ptr<const vw::ImageView> light_buffer) {

    vw::execute_image_barrier_general_to_sampled(
        buffer, light_buffer->image(),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR);
}
