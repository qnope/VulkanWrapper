use crate::sys::bindings::{VwAttachment, VwString, VwFormat, VwImageLayout};
use std::ffi::CString;

pub struct Attachment {
    id: CString,
    format: VwFormat,
    final_layout: VwImageLayout,
}

pub struct AttachmentBuilder {
    id: CString,
    format: VwFormat,
    final_layout: VwImageLayout,
}

impl AttachmentBuilder {
    pub fn new(id: &str) -> AttachmentBuilder {
        AttachmentBuilder {
            id: CString::new(id).unwrap(),
            format: VwFormat::B8G8R8A8_SRGB,
            final_layout: VwImageLayout::Undefined,
        }
    }

    pub fn with_format(mut self, format: VwFormat) -> AttachmentBuilder {
        self.format = format;
        self
    }

    pub fn with_final_layout(mut self, layout: VwImageLayout) -> AttachmentBuilder {
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
    pub fn to_raw(&self) -> VwAttachment {
        VwAttachment {
            id: VwString{string: self.id.as_ptr()},
            format: self.format,
            finalLayout: self.final_layout,
        }
    }
}
