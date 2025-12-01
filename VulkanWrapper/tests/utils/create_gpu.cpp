#include "create_gpu.hpp"

#include "VulkanWrapper/Vulkan/DeviceFinder.h"

namespace vw::tests {

GPU create_gpu() {
    auto instance = InstanceBuilder()
                        .setDebug()
                        .setApiVersion(ApiVersion::e13)
                        .build();

    auto device = instance.findGpu()
                      .with_queue(vk::QueueFlagBits::eGraphics)
                      .with_synchronization_2()
                      .with_dynamic_rendering()
                      .build();

    return GPU{std::move(instance), std::move(device)};
}

} // namespace vw::tests
