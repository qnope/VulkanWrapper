use crate::sys::bindings::{self, vw_Semaphore};
use crate::vulkan::device::Device;

pub struct Semaphore<'a> {
    _device: &'a Device<'a>,
    ptr: *mut vw_Semaphore,
}

pub struct SemaphoreBuilder<'a> {
    device: &'a Device<'a>,
}

impl<'a> SemaphoreBuilder<'a> {
    pub fn new(device: &'a Device<'a>) -> SemaphoreBuilder<'a> {
        SemaphoreBuilder { device: device }
    }

    pub fn build(self) -> Semaphore<'a> {
        unsafe {
            Semaphore {
                _device: self.device,
                ptr: bindings::vw_create_semaphore(self.device.as_ptr()),
            }
        }
    }
}

impl<'a> Semaphore<'a> {
    pub fn as_ptr(&self) -> *const vw_Semaphore {
        self.ptr
    }
}

impl<'a> Drop for Semaphore<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_semaphore(self.ptr);
        }
    }
}
