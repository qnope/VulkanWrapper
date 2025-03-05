use std::marker::PhantomData;

use crate::sys::bindings::{self, vw_Fence};
use crate::vulkan::device::Device;

#[repr(transparent)]
pub struct Fence<'a> {
    ptr: *mut vw_Fence,
    _marker: PhantomData<&'a Device<'a>>,
}

impl<'a> Fence<'a> {
    pub fn new(fence: *mut vw_Fence, _device: &'a Device) -> Fence<'a> {
        Fence {
            ptr: fence,
            _marker: PhantomData,
        }
    }
    pub fn wait(&self) {
        unsafe {
            bindings::vw_wait_fence(self.ptr);
        }
    }

    pub fn as_ptr(&self) -> *const vw_Fence {
        self.ptr
    }

    pub fn reset(&self) {
        unsafe {
            bindings::vw_reset_fence(self.ptr);
        }
    }
}

impl<'a> Drop for Fence<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_fence(self.ptr);
        }
    }
}
