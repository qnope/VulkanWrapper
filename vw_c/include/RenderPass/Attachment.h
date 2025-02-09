#pragma once

#include <vulkan/vulkan.h>

extern "C" {
struct vw_Attachment {
    const char *id;
    VkFormat format;
    VkImageLayout finalLayout;
};
}
