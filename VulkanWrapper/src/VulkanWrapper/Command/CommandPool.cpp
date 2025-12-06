#include "VulkanWrapper/Command/CommandPool.h"

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

    auto [result, commandBuffers] =
        m_device->handle().allocateCommandBuffers(info);

    if (result != vk::Result::eSuccess) {
        throw CommandBufferAllocationException(std::source_location::current());
    }

    return commandBuffers;
}

CommandPoolBuilder::CommandPoolBuilder(std::shared_ptr<const Device> device)
    : m_device{std::move(device)} {}

CommandPool CommandPoolBuilder::build() && {
    auto info = vk::CommandPoolCreateInfo().setQueueFamilyIndex(0);

    auto [result, pool] = m_device->handle().createCommandPoolUnique(info);

    if (result != vk::Result::eSuccess) {
        throw CommandPoolCreationException(std::source_location::current());
    }

    return {m_device, std::move(pool)};
}
} // namespace vw
