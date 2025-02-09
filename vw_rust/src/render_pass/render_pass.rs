use super::subpass::Subpass;
use crate::sys::bindings::{self, vw_RenderPass};
use crate::vulkan::device::Device;

pub struct RenderPass<'a> {
    _device: &'a Device<'a>,
    ptr: *mut vw_RenderPass,
}

pub struct RenderPassBuilder<'a> {
    device: &'a Device<'a>,
    subpasses: Vec<Subpass>,
}

impl<'a> RenderPassBuilder<'a> {
    pub fn new(device: &'a Device) -> RenderPassBuilder<'a> {
        RenderPassBuilder {
            device: device,
            subpasses: vec![],
        }
    }

    pub fn add_subpass(mut self, subpass: Subpass) -> RenderPassBuilder<'a> {
        self.subpasses.push(subpass);
        self
    }

    pub fn build(self) -> RenderPass<'a> {
        let mut c_subpasses: Vec<_> = self.subpasses.iter().map(|x| x.as_mut_ptr()).collect();
        unsafe {
            RenderPass {
                _device: self.device,
                ptr: bindings::vw_create_render_pass(
                    self.device.as_ptr(),
                    c_subpasses.as_mut_ptr(),
                    c_subpasses.len() as i32,
                ),
            }
        }
    }
}

impl<'a> RenderPass<'a> {
    pub fn as_ptr(&self) -> *const vw_RenderPass {
        self.ptr
    }
}

impl<'a> Drop for RenderPass<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_render_pass(self.ptr);
        }
    }
}
