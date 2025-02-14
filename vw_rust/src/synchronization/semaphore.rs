use std::marker::PhantomData;

use crate::sys::bindings::{self, vw_Semaphore, vw_semaphore_handle, VkSemaphore};
use crate::vulkan::device::Device;

#[repr(transparent)]
pub struct Semaphore<'a> {
    ptr: *mut vw_Semaphore,
    _marker: PhantomData<&'a Device<'a>>,
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
                ptr: bindings::vw_create_semaphore(self.device.as_ptr()),
                _marker: PhantomData,
            }
        }
    }
}

impl<'a> Semaphore<'a> {
    pub fn as_ptr(&self) -> *const vw_Semaphore {
        self.ptr
    }

    pub fn handle(&self) -> VkSemaphore {
        unsafe { vw_semaphore_handle(self.ptr) }
    }
}

impl<'a> Drop for Semaphore<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_semaphore(self.ptr);
        }
    }
}
