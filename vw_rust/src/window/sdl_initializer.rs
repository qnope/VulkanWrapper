use crate::sys::bindings;

pub struct SdlInitializer {
    pub ptr: *mut bindings::vw_SDL_Initializer,
}

impl SdlInitializer {
    pub fn new() -> SdlInitializer {
        unsafe {
            SdlInitializer {
                ptr: bindings::vw_create_SDL_Initializer(),
            }
        }
    }
}

impl Drop for SdlInitializer {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_SDL_Initializer(self.ptr);
        }
    }
}
