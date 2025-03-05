use crate::command::command_buffer::CommandBuffer;
use crate::synchronization::fence::Fence;
use crate::sys::bindings::{
    self, vw_Queue, VkPipelineStageFlagBits, VkSemaphore, VwQueueSubmitArguments,
};
use crate::vulkan::device::Device;

pub struct Queue<'a> {
    ptr: *mut vw_Queue,
    device: &'a Device<'a>,
}

impl<'a> Queue<'a> {
    pub fn new(_device: &'a Device<'a>, ptr: *mut vw_Queue) -> Queue<'a> {
        Queue {
            ptr: ptr,
            device: _device,
        }
    }

    pub fn submit(
        &self,
        cmd_buffers: &[CommandBuffer<'a>],
        wait_stages: &[VkPipelineStageFlagBits],
        wait_semaphores: &[VkSemaphore],
        signal_semaphores: &[VkSemaphore],
    ) -> Fence<'a> {
        let arguments = VwQueueSubmitArguments {
            command_buffers: cmd_buffers.as_ptr() as *const _,
            command_buffer_count: cmd_buffers.len() as u32,
            wait_stages: wait_stages.as_ptr() as *const _,
            wait_stage_count: wait_stages.len() as u32,
            wait_semaphores: wait_semaphores.as_ptr(),
            wait_semaphore_count: wait_semaphores.len() as u32,
            signal_semaphores: signal_semaphores.as_ptr(),
            signal_semaphores_count: signal_semaphores.len() as u32,
        };
        unsafe {
            let fence = bindings::vw_queue_submit(self.ptr, &arguments);
            Fence::new(fence, self.device)
        }
    }
}
