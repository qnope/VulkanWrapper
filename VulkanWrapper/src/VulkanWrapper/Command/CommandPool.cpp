#include "VulkanWrapper/Command/CommandPool.h"

#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
CommandPool::CommandPool(const Device &device, vk::UniqueCommandPool pool)
    : ObjectWithUniqueHandle<vk::UniqueCommandPool>(std::move(pool))
    , m_device(&device) {}

std::vector<vk::CommandBuffer> CommandPool::allocate(std::size_t number) {
    const auto info = vk::CommandBufferAllocateInfo()
                          .setCommandPool(handle())
                          .setCommandBufferCount(number)
                          .setLevel(vk::CommandBufferLevel::ePrimary);

    auto [result, commandBuffers] =
        m_device->handle().allocateCommandBuffers(info);

    if (result != vk::Result::eSuccess)
        throw CommandBufferAllocationException(std::source_location::current());

    return commandBuffers;
}

CommandPoolBuilder::CommandPoolBuilder(const Device &device)
    : m_device{device} {}

CommandPool CommandPoolBuilder::build() && {
    auto info = vk::CommandPoolCreateInfo().setQueueFamilyIndex(0);

    auto [result, pool] = m_device.handle().createCommandPoolUnique(info);

    if (result != vk::Result::eSuccess)
        throw CommandPoolCreationException(std::source_location::current());

    return {m_device, std::move(pool)};
}
} // namespace vw
