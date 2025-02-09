use crate::sys::bindings::{self, vw_CommandPool};
use crate::vulkan::device::Device;

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

impl<'a> Drop for CommandPool<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_command_pool(self.ptr);
        }
    }
}
