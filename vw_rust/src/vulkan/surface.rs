use crate::sys::bindings;
use crate::vulkan::instance::Instance;
use crate::window::window::Window;

pub struct Surface<'a, 'b, 'c> {
    _window: &'b Window<'a>,
    _instance: &'c Instance,
    ptr: *mut bindings::vw_Surface,
}

impl<'a, 'b, 'c> Surface<'a, 'b, 'c> {
    pub fn new(
        window: &'b Window<'a>,
        instance: &'c Instance,
        ptr: *mut bindings::vw_Surface,
    ) -> Surface<'a, 'b, 'c> {
        Surface {
            _window: window,
            _instance: instance,
            ptr: ptr,
        }
    }
}

impl<'a, 'b, 'c> Drop for Surface<'_, '_, '_> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_surface(self.ptr);
        }
    }
}
