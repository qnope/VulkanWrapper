#include "VulkanWrapper/Synchronization/Fence.h"

#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

Fence::Fence(Device &device, vk::UniqueFence fence)
    : ObjectWithUniqueHandle<vk::UniqueFence>{std::move(fence)}
    , m_device{device} {}

void Fence::wait() const {
    m_device.handle().waitForFences(handle(), true,
                                    std::numeric_limits<uint64_t>::max());
}

void Fence::reset() const { m_device.handle().resetFences(handle()); }

Fence FenceBuilder::build(Device &device) && {
    const auto info =
        vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);

    auto [result, fence] = device.handle().createFenceUnique(info);

    return Fence{device, std::move(fence)};
}
} // namespace vw
