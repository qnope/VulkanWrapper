#pragma once

#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/Instance.h"

namespace vw::tests {

struct GPU {
    Instance instance;
    Device device;
    Allocator allocator;
};

GPU create_gpu();

} // namespace vw::tests
