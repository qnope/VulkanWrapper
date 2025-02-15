use super::sdl_initializer::*;
use crate::sys::array_const_string::ArrayConstString;
use crate::sys::bindings::{self, VwWindowCreateArguments};
use crate::vulkan::device::Device;
use crate::vulkan::instance::Instance;
use crate::vulkan::surface::Surface;
use crate::vulkan::swapchain::Swapchain;
use std::ffi::CString;

pub struct Window<'a> {
    _initializer: &'a SdlInitializer,
    ptr: *mut bindings::vw_Window,
}

pub struct WindowBuilder<'a> {
    initializer: &'a SdlInitializer,
    title: String,
    width: i32,
    height: i32,
}

impl<'a> WindowBuilder<'a> {
    pub fn new(initializer: &'a SdlInitializer) -> WindowBuilder<'a> {
        WindowBuilder {
            initializer: &initializer,
            title: String::new(),
            width: 0,
            height: 0,
        }
    }

    pub fn with_title(mut self, title: &str) -> WindowBuilder<'a> {
        self.title = String::from(title);
        self
    }

    pub fn sized(mut self, width: i32, height: i32) -> WindowBuilder<'a> {
        self.width = width;
        self.height = height;
        self
    }

    pub fn build(self) -> Window<'a> {
        let title = CString::new(self.title).unwrap();
        let arguments = VwWindowCreateArguments {
            initializer: self.initializer.ptr,
            width: self.width,
            height: self.height,
            title: title.as_ptr(),
        };
        unsafe {
            Window {
                _initializer: &self.initializer,
                ptr: bindings::vw_create_window(&arguments),
            }
        }
    }
}

impl<'a> Window<'a> {
    pub fn get_required_extensions(&self) -> Vec<String> {
        unsafe {
            let required_extensions = ArrayConstString {
                c_array: bindings::vw_get_required_extensions_from_window(self.ptr),
            };
            return required_extensions.to_vec();
        }
    }

    pub fn create_surface(&'a self, instance: &'a Instance) -> Surface<'a> {
        unsafe {
            Surface::new(
                self,
                instance,
                bindings::vw_create_surface_from_window(self.ptr, instance.as_ptr()),
            )
        }
    }

    pub fn create_swapchain(&'a self, device: &'a Device, surface: &'a Surface) -> Swapchain<'a> {
        unsafe {
            let ptr = bindings::vw_create_swapchain_from_window(
                self.ptr,
                device.as_ptr(),
                surface.as_ptr(),
            );
            return Swapchain::new(device, surface, ptr);
        }
    }

    pub fn is_close_requested(&self) -> bool {
        unsafe {
            return bindings::vw_is_close_window_requested(self.ptr);
        }
    }

    pub fn update(&self) {
        unsafe {
            bindings::vw_update_window(self.ptr);
        }
    }
}

impl<'a> Drop for Window<'_> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_window(self.ptr);
        }
    }
}
