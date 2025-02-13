use std::os::raw::c_void;

use crate::sys::bindings::{self, vw_CommandPool};
use crate::vulkan::device::Device;

use super::command_buffer::CommandBuffer;

pub struct CommandPool<'a> {
    _device: &'a Device<'a>,
    ptr: *mut vw_CommandPool,
}

pub struct CommandPoolBuilder<'a> {
    device: &'a Device<'a>,
}

impl<'a> CommandPoolBuilder<'a> {
    pub fn new(device: &'a Device) -> CommandPoolBuilder<'a> {
        CommandPoolBuilder { device: device }
    }

    pub fn build(self) -> CommandPool<'a> {
        unsafe {
            CommandPool {
                _device: self.device,
                ptr: bindings::vw_create_command_pool(self.device.as_ptr()),
            }
        }
    }
}

impl<'a> CommandPool<'a> {
    pub fn allocate(&'a self, number: i32) -> Vec<CommandBuffer<'a>> {
        let c_array = unsafe { bindings::vw_command_pool_allocate(self.ptr, number) };
        let mut result = vec![];
        for i in 0..c_array.size {
            let cmd_buffer = unsafe { *c_array.commandBuffers.add(i as usize) };
            result.push(CommandBuffer::new(self, cmd_buffer));
        }
        unsafe { libc::free(c_array.commandBuffers as *mut c_void) };
        result
    }
}

impl<'a> Drop for CommandPool<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_command_pool(self.ptr);
        }
    }
}
