use crate::sys::bindings;
use std::ffi::CString;

pub struct Instance {
    pub ptr: *mut bindings::vw_Instance,
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

        let extensions = bindings::ArrayConstString {
            array: data.as_mut_ptr(),
            size: data.len() as i32,
        };

        unsafe {
            Instance {
                ptr: bindings::vw_create_instance(self.debug_mode, extensions),
            }
        }
    }
}

impl Drop for Instance {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_instance(self.ptr);
        }
    }
}
