use crate::sys::bindings;
use crate::vulkan::{device::Device, surface::Surface};

pub struct Swapchain<'a> {
    _device: &'a Device<'a>,
    _surface: &'a Surface<'a>,
    ptr: *mut bindings::vw_Swapchain,
}

impl<'a> Swapchain<'a> {
    pub fn new(
        device: &'a Device,
        surface: &'a Surface,
        ptr: *mut bindings::vw_Swapchain,
    ) -> Swapchain<'a> {
        Swapchain {
            _surface: surface,
            _device: device,
            ptr: ptr,
        }
    }
}

impl<'a> Drop for Swapchain<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_swapchain(self.ptr);
        }
    }
}
