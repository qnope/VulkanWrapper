#include "RayTracing.h"

#include <VulkanWrapper/Image/Framebuffer.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Memory/Barrier.h>

enum { Color, Position, Normal, Tangeant, BiTangeant, Light, Depth };

void RayTracingPass::execute(vk::CommandBuffer command_buffer,
                             const vw::Framebuffer &framebuffer) {

    vw::execute_image_barrier_general_to_sampled(
        command_buffer, framebuffer.image_view(Light)->image(),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR);
}
