use super::attachment::Attachment;
use crate::sys::bindings::{self, vw_AttachmentSubpass, vw_Subpass, VkImageLayout};

pub struct Subpass {
    ptr: *mut vw_Subpass,
}

pub struct SubpassBuilder {
    attachments: Vec<(Attachment, VkImageLayout)>,
}

impl SubpassBuilder {
    pub fn new() -> SubpassBuilder {
        SubpassBuilder {
            attachments: vec![],
        }
    }

    pub fn add_color_attachment(
        mut self,
        attachment: Attachment,
        layout: VkImageLayout,
    ) -> SubpassBuilder {
        self.attachments.push((attachment, layout));
        self
    }

    pub fn build(self) -> Subpass {
        let c_attachments: Vec<_> = self
            .attachments
            .iter()
            .map(|(attachment, layout)| vw_AttachmentSubpass {
                attachment: attachment.to_raw(),
                currentLayout: *layout,
            })
            .collect();
        unsafe {
            Subpass {
                ptr: bindings::vw_create_subpass(
                    c_attachments.as_ptr(),
                    c_attachments.len() as i32,
                ),
            }
        }
    }
}

impl Subpass {
    pub fn as_mut_ptr(&self) -> *mut vw_Subpass {
        self.ptr
    }
}

impl Drop for Subpass {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_subpass(self.ptr);
        }
    }
}
