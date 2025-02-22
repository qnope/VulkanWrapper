#pragma once

#include "../utils/utils.h"
#include "../Vulkan/enums.h"
#include <vulkan/vulkan.h>

extern "C" {
struct VwAttachment {
    VwString id;
    VwFormat format;
    VwImageLayout finalLayout;
};
}
