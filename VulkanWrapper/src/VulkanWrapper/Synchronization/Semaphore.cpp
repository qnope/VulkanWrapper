#include "VulkanWrapper/Synchronization/Semaphore.h"

#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
SemaphoreBuilder::SemaphoreBuilder(std::shared_ptr<const Device> device)
    : m_device{std::move(device)} {}

Semaphore SemaphoreBuilder::build() {
    const auto info = vk::SemaphoreCreateInfo();
    auto semaphore = check_vk(m_device->handle().createSemaphoreUnique(info),
                              "Failed to create semaphore");
    return Semaphore{std::move(semaphore)};
}
} // namespace vw
