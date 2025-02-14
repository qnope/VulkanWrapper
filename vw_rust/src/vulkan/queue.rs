use crate::command::command_buffer::CommandBuffer;
use crate::synchronization::fence::Fence;
use crate::sys::bindings::{
    self, vw_Queue, VkPipelineStageFlagBits, VkSemaphore, VwQueueSubmitArguments,
};
use crate::vulkan::device::Device;
use std::marker::PhantomData;
use std::ptr;

pub struct Queue<'a> {
    ptr: *const vw_Queue,
    _marker: PhantomData<&'a Device<'a>>,
}

impl<'a> Queue<'a> {
    pub fn new(_device: &'a Device<'a>, ptr: *const vw_Queue) -> Queue<'a> {
        Queue {
            ptr: ptr,
            _marker: PhantomData,
        }
    }

    pub fn submit(
        &self,
        cmd_buffers: &[CommandBuffer<'a>],
        wait_stages: &[VkPipelineStageFlagBits],
        wait_semaphores: &[VkSemaphore],
        signal_semaphores: &[VkSemaphore],
        fence: Option<&Fence<'a>>,
    ) {
        let arguments = VwQueueSubmitArguments {
            command_buffers: cmd_buffers.as_ptr() as *const _,
            command_buffer_count: cmd_buffers.len() as u32,
            wait_stages: wait_stages.as_ptr() as *const _,
            wait_stage_count: wait_stages.len() as u32,
            wait_semaphores: wait_semaphores.as_ptr(),
            wait_semaphore_count: wait_semaphores.len() as u32,
            signal_semaphores: signal_semaphores.as_ptr(),
            signal_semaphores_count: signal_semaphores.len() as u32,
            fence: match fence {
                Some(f) => f.as_ptr(),
                _ => ptr::null(),
            },
        };
        unsafe {
            bindings::vw_queue_submit(self.ptr, &arguments);
        }
    }
}
