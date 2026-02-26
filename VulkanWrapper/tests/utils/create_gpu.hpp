#pragma once

#include "VulkanWrapper/3rd_party.h"
#include <memory>
#include <vulkan/vulkan.hpp>
import vw;

namespace vw::tests {

struct GPU {
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;

    vw::Queue& queue() { return device->graphicsQueue(); }
};

GPU& create_gpu();

} // namespace vw::tests
