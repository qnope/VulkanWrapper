use super::command_pool::CommandPool;
use crate::pipeline::pipeline::Pipeline;
use crate::{
    image::framebuffer::Framebuffer, render_pass::render_pass::RenderPass, sys::bindings::*,
};
use std::ptr;

#[repr(transparent)]
pub struct CommandBuffer<'a> {
    cmd_buffer: VkCommandBuffer,
    _marker: std::marker::PhantomData<&'a CommandPool<'a>>,
}

pub struct PipelineBoundCommandBufferRecorder<'a> {
    cmd_buffer: &'a CommandBuffer<'a>,
}

pub struct RenderPassCommandBufferRecorder<'a> {
    cmd_buffer: &'a CommandBuffer<'a>,
}

pub struct CommandBufferRecorder<'a> {
    cmd_buffer: &'a CommandBuffer<'a>,
}

impl<'a> PipelineBoundCommandBufferRecorder<'a> {
    pub fn draw(
        &'a self,
        vertex_count: u32,
        instance_count: u32,
        first_vertex: u32,
        first_instance: u32,
    ) -> &'a Self {
        unsafe {
            vkCmdDraw(
                self.cmd_buffer.cmd_buffer,
                vertex_count,
                instance_count,
                first_vertex,
                first_instance,
            );
        }
        self
    }
}

impl<'a> RenderPassCommandBufferRecorder<'a> {
    pub fn bind_graphics_pipeline(
        &'a self,
        pipeline: &'a Pipeline<'a>,
    ) -> PipelineBoundCommandBufferRecorder<'a> {
        unsafe {
            vkCmdBindPipeline(
                self.cmd_buffer.cmd_buffer,
                VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.handle(),
            );
        }
        PipelineBoundCommandBufferRecorder {
            cmd_buffer: self.cmd_buffer,
        }
    }
}

impl<'a> Drop for RenderPassCommandBufferRecorder<'a> {
    fn drop(&mut self) {
        let info = VkSubpassEndInfo {
            sType: VkStructureType::VK_STRUCTURE_TYPE_SUBPASS_END_INFO,
            pNext: ptr::null(),
        };
        unsafe {
            vkCmdEndRenderPass2(self.cmd_buffer.cmd_buffer, &info);
        }
    }
}

impl<'a> CommandBufferRecorder<'a> {
    pub fn new(cmd_buffer: &'a CommandBuffer<'a>) -> Self {
        let begin_info = VkCommandBufferBeginInfo {
            sType: VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            pNext: ptr::null(),
            flags: 0,
            pInheritanceInfo: ptr::null(),
        };
        unsafe {
            vkBeginCommandBuffer(cmd_buffer.cmd_buffer, &begin_info);
        }
        return CommandBufferRecorder {
            cmd_buffer: cmd_buffer,
        };
    }

    pub fn begin_render_pass(
        &'a self,
        render_pass: &RenderPass<'a>,
        framebuffer: &'a Framebuffer<'a>,
    ) -> RenderPassCommandBufferRecorder<'a> {
        let clear_color = VkClearColorValue {
            float32: [0.0, 0.0, 0.0, 1.0],
        };
        let color = VkClearValue { color: clear_color };

        let render_pass_info = VkRenderPassBeginInfo {
            sType: VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            pNext: ptr::null(),
            renderPass: render_pass.handle(),
            framebuffer: framebuffer.handle(),
            renderArea: VkRect2D {
                offset: VkOffset2D { x: 0, y: 0 },
                extent: VkExtent2D {
                    width: framebuffer.width(),
                    height: framebuffer.height(),
                },
            },
            clearValueCount: 1,
            pClearValues: &color,
        };

        let subpass_info = VkSubpassBeginInfo {
            sType: VkStructureType::VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
            pNext: ptr::null(),
            contents: VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE,
        };

        unsafe {
            vkCmdBeginRenderPass2(self.cmd_buffer.cmd_buffer, &render_pass_info, &subpass_info);
        }
        RenderPassCommandBufferRecorder {
            cmd_buffer: &self.cmd_buffer,
        }
    }
}

impl<'a> Drop for CommandBufferRecorder<'a> {
    fn drop(&mut self) {
        unsafe {
            vkEndCommandBuffer(self.cmd_buffer.cmd_buffer);
        }
    }
}

impl<'a> CommandBuffer<'a> {
    pub fn new(_pool: &'a CommandPool<'a>, cmd_buffer: VkCommandBuffer) -> Self {
        CommandBuffer {
            cmd_buffer: cmd_buffer,
            _marker: std::marker::PhantomData,
        }
    }
}
