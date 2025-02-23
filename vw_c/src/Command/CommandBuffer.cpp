#include "Command/CommandBuffer.h"

#include <VulkanWrapper/Command/CommandBuffer.h>

vw::CommandBufferRecorder *
vw_create_command_buffer_recorder(VkCommandBuffer cmd_buffer) {
    return new vw::CommandBufferRecorder{vk::CommandBuffer{cmd_buffer}};
}

vw::RenderPassCommandBufferRecorder *
vw_begin_render_pass(vw::CommandBufferRecorder *recorder,
                     const vw::RenderPass *render_pass,
                     const vw::Framebuffer *framebuffer) {
    return new vw::RenderPassCommandBufferRecorder{
        recorder->begin_render_pass(*render_pass, *framebuffer)};
}

vw::PipelineBoundCommandBufferRecorder *
vw_bind_graphics_pipeline(vw::RenderPassCommandBufferRecorder *recorder,
                          const vw::Pipeline *pipeline) {
    return new vw::PipelineBoundCommandBufferRecorder{
        recorder->bind_graphics_pipeline(*pipeline)};
}

void vw_draw(vw::PipelineBoundCommandBufferRecorder *recorder,
             int number_vertex, int number_instance, int first_vertex,
             int first_instance) {
    recorder->draw(number_vertex, number_instance, first_vertex,
                   first_instance);
}

void vw_destroy_command_buffer_recorder(vw::CommandBufferRecorder *recorder) {
    delete recorder;
}

void vw_destroy_render_pass_command_buffer_recorder(
    vw::RenderPassCommandBufferRecorder *recorder) {
    delete recorder;
}

void vw_destroy_pipeline_bound_command_buffer_recorder(
    vw::PipelineBoundCommandBufferRecorder *recorder) {
    delete recorder;
}
