use crate::image::image_view::ImageView;
use crate::render_pass::render_pass::RenderPass;
use crate::sys::bindings::{self, vw_Framebuffer, VkFramebuffer, VwFramebufferCreateArguments};
use crate::vulkan::device::Device;
use std::rc::Rc;

pub struct Framebuffer<'a> {
    _image_view: Vec<Rc<ImageView<'a>>>,
    ptr: *mut vw_Framebuffer,
}

pub struct FramebufferBuilder<'a> {
    device: &'a Device<'a>,
    render_pass: &'a RenderPass<'a>,
    width: i32,
    height: i32,
    attachments: Vec<Rc<ImageView<'a>>>,
}

impl<'a> FramebufferBuilder<'a> {
    pub fn new(
        device: &'a Device,
        render_pass: &'a RenderPass,
        width: i32,
        height: i32,
    ) -> FramebufferBuilder<'a> {
        FramebufferBuilder {
            device: device,
            render_pass: render_pass,
            width: width,
            height: height,
            attachments: vec![],
        }
    }

    pub fn with_attachment(mut self, attachment: Rc<ImageView<'a>>) -> FramebufferBuilder<'a> {
        self.attachments.push(attachment);
        self
    }

    pub fn build(self) -> Framebuffer<'a> {
        let mut attachments: Vec<_> = self.attachments.iter().map(|x| x.as_ptr()).collect();
        let arguments = VwFramebufferCreateArguments {
            device: self.device.as_ptr(),
            render_pass: self.render_pass.as_ptr(),
            image_views: attachments.as_mut_ptr(),
            number_image_views: attachments.len() as i32,
            width: self.width,
            height: self.height,
        };

        Framebuffer {
            _image_view: self.attachments,
            ptr: unsafe { bindings::vw_create_framebuffer(&arguments) },
        }
    }
}

impl<'a> Framebuffer<'a> {
    pub fn width(&self) -> u32 {
        unsafe { bindings::vw_framebuffer_width(self.ptr) }
    }

    pub fn height(&self) -> u32 {
        unsafe { bindings::vw_framebuffer_height(self.ptr) }
    }

    pub fn handle(&self) -> VkFramebuffer {
        unsafe { bindings::vw_framebuffer_handle(self.ptr) }
    }
}

impl<'a> Drop for Framebuffer<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_framebuffer(self.ptr);
        }
    }
}
