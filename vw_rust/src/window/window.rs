use super::sdl_initializer::*;
use crate::sys::array_const_string::ArrayConstString;
use crate::sys::bindings;
use crate::vulkan::instance::Instance;
use crate::vulkan::surface::Surface;
use std::ffi::CString;

pub struct Window<'a> {
    _initializer: &'a SdlInitializer,
    ptr: *mut bindings::vw_Window,
}

pub struct WindowBuilder<'a> {
    _initializer: &'a SdlInitializer,
    title: String,
    width: i32,
    height: i32,
}

impl<'a> WindowBuilder<'a> {
    pub fn new(initializer: &'a SdlInitializer) -> WindowBuilder<'a> {
        WindowBuilder {
            _initializer: &initializer,
            title: String::new(),
            width: 0,
            height: 0,
        }
    }

    pub fn with_title(mut self, title: String) -> WindowBuilder<'a> {
        self.title = title;
        self
    }

    pub fn sized(mut self, width: i32, height: i32) -> WindowBuilder<'a> {
        self.width = width;
        self.height = height;
        self
    }

    pub fn build(self) -> Window<'a> {
        let title = CString::new(self.title).unwrap();

        unsafe {
            Window {
                _initializer: &self._initializer,
                ptr: bindings::vw_create_window(
                    self._initializer.ptr,
                    self.width,
                    self.height,
                    title.as_ptr(),
                ),
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
            Surface::new(self, instance, bindings::vw_create_surface_from_window(self.ptr, instance.ptr))
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
