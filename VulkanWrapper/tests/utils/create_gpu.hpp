#pragma once

#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/Instance.h"
#include <memory>

namespace vw::tests {

struct GPU {
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;

    vw::Queue& queue() { return device->graphicsQueue(); }
};

GPU& create_gpu();

} // namespace vw::tests
