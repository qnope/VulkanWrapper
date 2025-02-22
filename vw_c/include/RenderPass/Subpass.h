#pragma once

#include "../Vulkan/enums.h"
#include "Attachment.h"

namespace vw {
class Subpass;
}

extern "C" {
struct VwAttachmentSubpass {
    VwAttachment attachment;
    VwImageLayout currentLayout;
};

struct VwSubpassCreateArguments {
    const VwAttachmentSubpass *attachments;
    int attachment_count;
};

vw::Subpass *vw_create_subpass(const VwSubpassCreateArguments *arguments);

void vw_destroy_subpass(vw::Subpass *subpass);
}
