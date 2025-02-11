use crate::sys::bindings::{self, vw_Image};
use crate::vulkan::device::Device;

pub struct Image<'a> {
    _device: &'a Device<'a>,
    ptr: *mut vw_Image,
}

impl<'a> Image<'a> {
    pub fn new(device: &'a Device<'a>, ptr: *mut vw_Image) -> Image<'a> {
        Image {
            _device: device,
            ptr,
        }
    }

    pub fn as_ptr(&self) -> *const vw_Image {
        self.ptr
    }
}

impl<'a> Drop for Image<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_image(self.ptr);
        }
    }
}
