#include "VulkanWrapper/Synchronization/Semaphore.h"

#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
SemaphoreBuilder::SemaphoreBuilder(std::shared_ptr<const Device> device)
    : m_device{std::move(device)} {}

Semaphore SemaphoreBuilder::build() && {
    const auto info = vk::SemaphoreCreateInfo();
    auto [result, semaphore] = m_device->handle().createSemaphoreUnique(info);
    if (result != vk::Result::eSuccess) {
        throw SemaphoreCreationException{std::source_location::current()};
    }
    return Semaphore{std::move(semaphore)};
}
} // namespace vw
