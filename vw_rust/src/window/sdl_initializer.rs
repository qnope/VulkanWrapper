pub enum VwSDLInitializer{}

#[link(name="VulkanWrapper_cd", kind="dylib")]
unsafe extern {
    fn vw_create_SDL_Initializer() -> *mut VwSDLInitializer;
    fn vw_destroy_SDL_Initializer(initializer: *mut VwSDLInitializer);
}

pub struct SdlInitializer {
    pub ptr: *mut VwSDLInitializer
}

impl SdlInitializer {
    pub fn new() -> SdlInitializer
    {
        unsafe {
            SdlInitializer {
                ptr: vw_create_SDL_Initializer()
            }
        }
    }
}

impl Drop for SdlInitializer {
    fn drop(&mut self) {
        unsafe {
            vw_destroy_SDL_Initializer(self.ptr);
        }
    }
}