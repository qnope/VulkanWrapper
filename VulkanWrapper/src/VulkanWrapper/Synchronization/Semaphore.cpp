#include "VulkanWrapper/Synchronization/Semaphore.h"

#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
Semaphore SemaphoreBuilder::build(Device &device) && {
    const auto info = vk::SemaphoreCreateInfo();
    auto [result, semaphore] = device.handle().createSemaphoreUnique(info);
    if (result != vk::Result::eSuccess)
        throw SemaphoreCreationException{std::source_location::current()};
    return Semaphore{std::move(semaphore)};
}
} // namespace vw
