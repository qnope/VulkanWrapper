use crate::image::image::Image;
use crate::sys::bindings::{self, vw_ImageView, VwImageViewCreateArguments, VwImageViewType};
use crate::vulkan::device::Device;
use std::rc::Rc;

pub struct ImageView<'a> {
    _device: &'a Device<'a>,
    _image: Rc<Image<'a>>,
    ptr: *mut vw_ImageView,
}

pub struct ImageViewBuilder<'a> {
    device: &'a Device<'a>,
    image: Rc<Image<'a>>,
    image_type: VwImageViewType,
}

impl<'a> ImageViewBuilder<'a> {
    pub fn new(device: &'a Device<'a>, image: Rc<Image<'a>>) -> ImageViewBuilder<'a> {
        ImageViewBuilder {
            device: device,
            image: image,
            image_type: VwImageViewType::Type2D,
        }
    }

    pub fn with_image_type(mut self, image_type: VwImageViewType) -> ImageViewBuilder<'a> {
        self.image_type = image_type;
        self
    }

    pub fn build(self) -> Rc<ImageView<'a>> {
        let arguments = VwImageViewCreateArguments {
            device: self.device.as_ptr(),
            image: self.image.as_ptr(),
            image_type: self.image_type,
        };

        let image_view = ImageView {
            _device: self.device,
            _image: self.image,
            ptr: unsafe { bindings::vw_create_image_view(&arguments) },
        };

        Rc::new(image_view)
    }
}

impl<'a> ImageView<'a> {
    pub fn as_ptr(&self) -> *const vw_ImageView {
        self.ptr
    }
}

impl<'a> Drop for ImageView<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_image_view(self.ptr);
        }
    }
}
