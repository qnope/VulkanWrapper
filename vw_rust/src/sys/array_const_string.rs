use std::os::raw::{c_int, c_char, c_void};
use std::ffi::CStr;
use libc::free;

#[repr(C)]
pub struct ArrayConstString {
    ptr: *mut *const c_char,
    size: c_int,
}

impl ArrayConstString
{
    pub fn to_vec(&self) -> Vec<String> {
        let mut result = vec![];
        unsafe {
            for i in 0..self.size {
                let string_pointer = (self.ptr).add(i.try_into().unwrap());
                let cstr = CStr::from_ptr(*string_pointer);
                let string = String::from_utf8_lossy(cstr.to_bytes()).to_string();
                result.push(string);
            }
        }
        result
    }
}

impl Drop for ArrayConstString
{
    fn drop(&mut self) {
        unsafe {
            free(self.ptr as *mut c_void);
        }
    }
}
