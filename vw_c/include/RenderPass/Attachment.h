#pragma once

#include <vulkan/vulkan.h>

extern "C" {
struct VwAttachment {
    const char *id;
    VkFormat format;
    VkImageLayout finalLayout;
};
}
