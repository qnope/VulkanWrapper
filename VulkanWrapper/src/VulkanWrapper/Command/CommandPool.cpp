#include "VulkanWrapper/Command/CommandPool.h"

#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
CommandPool::CommandPool(std::shared_ptr<const Device> device,
                         vk::UniqueCommandPool pool)
    : ObjectWithUniqueHandle<vk::UniqueCommandPool>(std::move(pool))
    , m_device(std::move(device)) {}

std::vector<vk::CommandBuffer> CommandPool::allocate(std::size_t number) {
    const auto info = vk::CommandBufferAllocateInfo()
                          .setCommandPool(handle())
                          .setCommandBufferCount(number)
                          .setLevel(vk::CommandBufferLevel::ePrimary);

    auto commandBuffers = check_vk(m_device->handle().allocateCommandBuffers(info),
                                    "Failed to allocate command buffers");

    return commandBuffers;
}

CommandPoolBuilder::CommandPoolBuilder(std::shared_ptr<const Device> device)
    : m_device{std::move(device)} {}

CommandPool CommandPoolBuilder::build() && {
    auto info = vk::CommandPoolCreateInfo().setQueueFamilyIndex(0);

    auto pool = check_vk(m_device->handle().createCommandPoolUnique(info),
                         "Failed to create command pool");

    return {m_device, std::move(pool)};
}
} // namespace vw
