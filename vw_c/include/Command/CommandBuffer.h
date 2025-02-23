#pragma once

#include <vulkan/vulkan.h>

namespace vw {
class PipelineBoundCommandBufferRecorder;
class RenderPassCommandBufferRecorder;
class CommandBufferRecorder;
class Pipeline;
class Framebuffer;
class RenderPass;
} // namespace vw

extern "C" {
vw::CommandBufferRecorder *
vw_create_command_buffer_recorder(VkCommandBuffer cmd_buffer);

vw::RenderPassCommandBufferRecorder *
vw_begin_render_pass(vw::CommandBufferRecorder *recorder,
                     const vw::RenderPass *render_pass,
                     const vw::Framebuffer *framebuffer);

vw::PipelineBoundCommandBufferRecorder *
vw_bind_graphics_pipeline(vw::RenderPassCommandBufferRecorder *recorder,
                          const vw::Pipeline *pipeline);

void vw_draw(vw::PipelineBoundCommandBufferRecorder *recorder,
             int number_vertex, int number_instance, int first_vertex,
             int first_instance);

void vw_destroy_command_buffer_recorder(vw::CommandBufferRecorder *recorder);

void vw_destroy_render_pass_command_buffer_recorder(
    vw::RenderPassCommandBufferRecorder *recorder);

void vw_destroy_pipeline_bound_command_buffer_recorder(
    vw::PipelineBoundCommandBufferRecorder *recorder);
}
