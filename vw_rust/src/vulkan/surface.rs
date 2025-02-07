use crate::sys::bindings;
use crate::vulkan::instance::Instance;
use crate::window::window::Window;

pub struct Surface<'a> {
    _window: &'a Window<'a>,
    _instance: &'a Instance,
    ptr: *mut bindings::vw_Surface,
}

impl<'a> Surface<'a> {
    pub fn new(
        window: &'a Window<'a>,
        instance: &'a Instance,
        ptr: *mut bindings::vw_Surface,
    ) -> Surface<'a> {
        Surface {
            _window: window,
            _instance: instance,
            ptr: ptr,
        }
    }
}

impl<'a, 'b, 'c> Drop for Surface<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_surface(self.ptr);
        }
    }
}
