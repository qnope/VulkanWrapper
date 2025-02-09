use crate::sys::bindings::{self, vw_PipelineLayout};
use crate::vulkan::device::Device;

pub struct PipelineLayout<'a> {
    _device: &'a Device<'a>,
    ptr: *mut vw_PipelineLayout,
}

pub struct PipelineLayoutBuilder<'a> {
    device: &'a Device<'a>,
}

impl<'a> PipelineLayoutBuilder<'a> {
    pub fn new(device: &'a Device) -> PipelineLayoutBuilder<'a> {
        PipelineLayoutBuilder { device: device }
    }

    pub fn build(self) -> PipelineLayout<'a> {
        unsafe {
            PipelineLayout {
                _device: self.device,
                ptr: bindings::vw_create_pipeline_layout(self.device.as_ptr()),
            }
        }
    }
}

impl<'a> PipelineLayout<'a> {
    pub fn as_ptr(&self) -> *const vw_PipelineLayout {
        self.ptr
    }
}

impl<'a> Drop for PipelineLayout<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_pipeline_layout(self.ptr);
        }
    }
}
