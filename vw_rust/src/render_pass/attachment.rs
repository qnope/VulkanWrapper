use crate::sys::bindings::{vw_Attachment, VkFormat, VkImageLayout};
use std::ffi::CString;

pub struct Attachment {
    id: CString,
    format: VkFormat,
    final_layout: VkImageLayout,
}

pub struct AttachmentBuilder {
    id: CString,
    format: VkFormat,
    final_layout: VkImageLayout,
}

impl AttachmentBuilder {
    pub fn new(id: &str) -> AttachmentBuilder {
        AttachmentBuilder {
            id: CString::new(id).unwrap(),
            format: VkFormat::VK_FORMAT_B8G8R8A8_SRGB,
            final_layout: VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
        }
    }

    pub fn with_format(mut self, format: VkFormat) -> AttachmentBuilder {
        self.format = format;
        self
    }

    pub fn with_final_layout(mut self, layout: VkImageLayout) -> AttachmentBuilder {
        self.final_layout = layout;
        self
    }

    pub fn build(self) -> Attachment {
        Attachment {
            id: self.id,
            format: self.format,
            final_layout: self.final_layout,
        }
    }
}

impl Attachment {
    pub fn to_raw(&self) -> vw_Attachment {
        vw_Attachment {
            id: self.id.as_ptr(),
            format: self.format,
            finalLayout: self.final_layout,
        }
    }
}
