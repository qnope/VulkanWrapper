use crate::image::image::Image;
use crate::synchronization::semaphore::Semaphore;
use crate::sys::bindings::{self, vw_Swapchain};
use crate::vulkan::{device::Device, surface::Surface};
use libc::c_void;
use std::rc::Rc;

pub struct Swapchain<'a> {
    device: &'a Device<'a>,
    _surface: &'a Surface<'a>,
    ptr: *mut vw_Swapchain,
}

impl<'a> Swapchain<'a> {
    pub fn new(device: &'a Device, surface: &'a Surface, ptr: *mut vw_Swapchain) -> Swapchain<'a> {
        Swapchain {
            _surface: surface,
            device: device,
            ptr: ptr,
        }
    }

    pub fn width(&self) -> i32 {
        unsafe {
            return bindings::vw_get_swapchain_width(self.ptr);
        }
    }

    pub fn height(&self) -> i32 {
        unsafe {
            return bindings::vw_get_swapchain_height(self.ptr);
        }
    }

    pub fn format(&self) -> bindings::VkFormat {
        unsafe {
            return bindings::vw_get_swapchain_format(self.ptr);
        }
    }

    pub fn acquire_next_image(&self, semaphore: &Semaphore) -> u64 {
        unsafe {
            return bindings::vw_swapchain_acquire_next_image(self.ptr, semaphore.as_ptr());
        }
    }

    pub fn images(&'a self) -> Vec<Rc<Image<'a>>> {
        unsafe {
            let image_array = bindings::vw_swapchain_get_images(self.ptr);
            let mut result = vec![];
            for i in 0..image_array.size {
                let image = Image::new(self.device, *image_array.images.add(i.try_into().unwrap()));
                result.push(Rc::new(image));
            }

            libc::free(image_array.images as *mut c_void);
            result
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
