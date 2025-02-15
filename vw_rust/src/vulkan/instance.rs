use super::device::DeviceFinder;
use crate::sys::bindings::{self, vw_Instance, VwInstanceCreateArguments};
use std::ffi::CString;

pub struct Instance {
    ptr: *mut vw_Instance,
}

pub struct InstanceBuilder {
    extensions: Vec<String>,
    debug_mode: bool,
}

impl InstanceBuilder {
    pub fn new() -> InstanceBuilder {
        InstanceBuilder {
            extensions: vec![],
            debug_mode: false,
        }
    }

    pub fn add_extension(mut self, extension: String) -> InstanceBuilder {
        self.extensions.push(extension);
        self
    }

    pub fn add_extensions(mut self, mut extensions: Vec<String>) -> InstanceBuilder {
        self.extensions.append(&mut extensions);
        self
    }

    pub fn build(self) -> Instance {
        let vec_c_string: Vec<_> = self
            .extensions
            .into_iter()
            .map(|x| CString::new(x).unwrap())
            .collect();

        let mut data: Vec<_> = vec_c_string.iter().map(|x| x.as_ptr()).collect();

        let arguments = VwInstanceCreateArguments {
            extensions: data.as_mut_ptr(),
            extensions_count: data.len() as i32,
            debug_mode: self.debug_mode
        };

        unsafe {
            Instance {
                ptr: bindings::vw_create_instance(&arguments),
            }
        }
    }
}

impl Instance {
    pub fn find_gpu(&self) -> DeviceFinder {
        return DeviceFinder::new(self);
    }

    pub fn as_ptr(&self) -> *const vw_Instance {
        self.ptr
    }
}

impl Drop for Instance {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_instance(self.ptr);
        }
    }
}
