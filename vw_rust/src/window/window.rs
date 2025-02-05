use crate::sys::array_const_string::ArrayConstString;
use super::sdl_initializer::*;

enum VwWindow{}

unsafe extern {
    fn vw_create_Window(initializer: *mut VwSDLInitializer) -> *mut VwWindow;

    fn vw_close_Window_requested(window: *mut VwWindow) -> bool;
    fn vw_update_Window(window: *mut VwWindow);
    fn vw_required_extensions_from_window(window: *const VwWindow) -> ArrayConstString;

    fn vw_destroy_Window(window: *mut VwWindow);
}

pub struct Window<'a> {
    _initializer:  &'a SdlInitializer,
    ptr: *mut VwWindow
}

impl<'a> Window<'a> {
    pub fn new(initializer: &'a SdlInitializer) -> Window<'a> {
        unsafe {
            Window {
                _initializer: &initializer,
                ptr: vw_create_Window(initializer.ptr)
            }
        }
    }

    pub fn get_required_extensions(&self) -> Vec<String> {
        unsafe {
            return vw_required_extensions_from_window(self.ptr).to_vec();
        }
    }

    pub fn is_close_requested(&self) -> bool {
        unsafe {
            return vw_close_Window_requested(self.ptr);
        }
    }

    pub fn update(&mut self) {
        unsafe {
            vw_update_Window(self.ptr);
        }
    }
}

impl<'a> Drop for Window<'_> {
    fn drop(&mut self) {
        unsafe {
            vw_destroy_Window(self.ptr);
        }
    }
}