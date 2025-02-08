use crate::sys::bindings::{self, vw_ShaderModule};
use crate::vulkan::device::Device;
use std::ffi::CString;

pub struct ShaderModule<'a> {
    _device: &'a Device<'a>,
    ptr: *mut vw_ShaderModule,
}

impl<'a> ShaderModule<'a> {
    pub fn create_from_spirv_file(device: &'a Device, path: &str) -> ShaderModule<'a> {
        let path_string = CString::new(path).unwrap();
        unsafe {
            let ptr = bindings::vw_create_shader_module_from_spirv_file(
                device.as_ptr(),
                path_string.as_ptr(),
            );
            ShaderModule {
                _device: device,
                ptr: ptr,
            }
        }
    }
}

impl<'a> Drop for ShaderModule<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_shader_module(self.ptr);
        }
    }
}