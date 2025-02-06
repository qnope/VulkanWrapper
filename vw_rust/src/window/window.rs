use crate::sys::array_const_string::ArrayConstString;
use super::sdl_initializer::*;
use crate::sys::bindings;

pub struct Window<'a> {
    _initializer:  &'a SdlInitializer,
    ptr: *mut bindings::vw_Window
}

impl<'a> Window<'a> {
    pub fn new(initializer: &'a SdlInitializer) -> Window<'a> {
        unsafe {
            Window {
                _initializer: &initializer,
                ptr: bindings::vw_create_Window(initializer.ptr)
            }
        }
    }

    pub fn get_required_extensions(&self) -> Vec<String> {
        unsafe {
            let required_extensions = ArrayConstString{c_array: bindings::vw_required_extensions_from_window(self.ptr)};
            return required_extensions.to_vec();
        }
    }

    pub fn is_close_requested(&self) -> bool {
        unsafe {
            return bindings::vw_close_Window_requested(self.ptr);
        }
    }

    pub fn update(&mut self) {
        unsafe {
            bindings::vw_update_Window(self.ptr);
        }
    }
}

impl<'a> Drop for Window<'_> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_Window(self.ptr);
        }
    }
}