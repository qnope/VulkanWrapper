module;
#include "VulkanWrapper/3rd_party.h"
#include <vulkan/vulkan.hpp>
#include <memory>
module vw;
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
