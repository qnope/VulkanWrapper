use std::marker::PhantomData;

use crate::sys::bindings::{self, vw_Fence};
use crate::vulkan::device::Device;

#[repr(transparent)]
pub struct Fence<'a> {
    ptr: *mut vw_Fence,
    _marker: PhantomData<&'a Device<'a>>,
}

pub struct FenceBuilder<'a> {
    device: &'a Device<'a>,
}

impl<'a> FenceBuilder<'a> {
    pub fn new(device: &'a Device<'a>) -> FenceBuilder<'a> {
        FenceBuilder { device: device }
    }

    pub fn build(self) -> Fence<'a> {
        unsafe {
            Fence {
                ptr: bindings::vw_create_fence(self.device.as_ptr()),
                _marker: PhantomData,
            }
        }
    }
}

impl<'a> Fence<'a> {
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
