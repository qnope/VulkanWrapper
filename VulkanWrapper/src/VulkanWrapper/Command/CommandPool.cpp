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

void CommandPool::reset(vk::CommandPoolResetFlags flags) {
    check_vk(m_device->handle().resetCommandPool(handle(), flags),
             "Failed to reset command pool");
}

CommandPoolBuilder::CommandPoolBuilder(std::shared_ptr<const Device> device)
    : m_device{std::move(device)} {}

CommandPoolBuilder &&CommandPoolBuilder::with_reset_command_buffer() && {
    m_flags |= vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    return std::move(*this);
}

CommandPoolBuilder &&CommandPoolBuilder::transient() && {
    m_flags |= vk::CommandPoolCreateFlagBits::eTransient;
    return std::move(*this);
}

CommandPool CommandPoolBuilder::build() && {
    auto info = vk::CommandPoolCreateInfo()
                    .setQueueFamilyIndex(0)
                    .setFlags(m_flags);

    auto pool = check_vk(m_device->handle().createCommandPoolUnique(info),
                         "Failed to create command pool");

    return {m_device, std::move(pool)};
}
} // namespace vw
