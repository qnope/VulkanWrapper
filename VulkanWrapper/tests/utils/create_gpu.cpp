#include "create_gpu.hpp"

#include "VulkanWrapper/Vulkan/DeviceFinder.h"

namespace vw::tests {

GPU &create_gpu() {
    // Intentionally leak GPU to avoid static destruction order issues
    // The OS will clean up memory when the test process exits
    static GPU *gpu = []() {
        auto instance =
            InstanceBuilder().setDebug().setApiVersion(ApiVersion::e13).build();

        auto device = instance->findGpu()
                          .with_queue(vk::QueueFlagBits::eGraphics)
                          .with_synchronization_2()
                          .with_dynamic_rendering()
                          .with_descriptor_indexing()
                          .build();

        auto allocator = AllocatorBuilder(instance, device).build();

        return new GPU{std::move(instance), std::move(device),
                       std::move(allocator)};
    }();

    return *gpu;
}

} // namespace vw::tests
