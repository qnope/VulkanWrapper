#pragma once

#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/Instance.h"

namespace vw::tests {

struct GPU {
    Instance instance;
    Device device;
};

GPU create_gpu();

} // namespace vw::tests
