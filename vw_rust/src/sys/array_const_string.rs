use libc::free;
use std::ffi::CStr;
use std::os::raw::c_void;

pub struct ArrayConstString {
    pub c_array: super::bindings::vw_ArrayConstString,
}

impl ArrayConstString {
    pub fn to_vec(&self) -> Vec<String> {
        let mut result = vec![];
        for i in 0..self.c_array.size {
            unsafe {
                let string_pointer = (self.c_array.array).add(i.try_into().unwrap());
                let cstr = CStr::from_ptr(*string_pointer);
                let string = String::from_utf8_lossy(cstr.to_bytes()).to_string();
                result.push(string);
            }
        }
        result
    }
}

impl Drop for ArrayConstString {
    fn drop(&mut self) {
        unsafe {
            free(self.c_array.array as *mut c_void);
        }
    }
}
