#pragma once

#include "Attachment.h"

namespace vw {
class Subpass;
}

extern "C" {
struct vw_AttachmentSubpass {
    vw_Attachment attachment;
    VkImageLayout currentLayout;
};
vw::Subpass *vw_create_subpass(const vw_AttachmentSubpass *attachments,
                               int size);

void vw_destroy_subpass(vw::Subpass *subpass);
}
