use super::attachment::Attachment;
use crate::sys::bindings::{
    self, vw_Subpass, VwAttachmentSubpass, VwImageLayout, VwSubpassCreateArguments,
};

pub struct Subpass {
    ptr: *mut vw_Subpass,
}

pub struct SubpassBuilder {
    attachments: Vec<(Attachment, VwImageLayout)>,
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
        layout: VwImageLayout,
    ) -> SubpassBuilder {
        self.attachments.push((attachment, layout));
        self
    }

    pub fn build(self) -> Subpass {
        let c_attachments: Vec<_> = self
            .attachments
            .iter()
            .map(|(attachment, layout)| VwAttachmentSubpass {
                attachment: attachment.to_raw(),
                currentLayout: *layout,
            })
            .collect();

        let arguments = VwSubpassCreateArguments {
            attachments: c_attachments.as_ptr(),
            attachment_count: c_attachments.len() as i32,
        };

        unsafe {
            Subpass {
                ptr: bindings::vw_create_subpass(&arguments),
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
