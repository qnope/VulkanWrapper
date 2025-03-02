#include "VulkanWrapper/Synchronization/Fence.h"

#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

Fence::Fence(const Device &device, vk::UniqueFence fence)
    : ObjectWithUniqueHandle<vk::UniqueFence>{std::move(fence)}
    , m_device{&device} {}

void Fence::wait() const {
    m_device->handle().waitForFences(handle(), true,
                                     std::numeric_limits<uint64_t>::max());
}

void Fence::reset() const { m_device->handle().resetFences(handle()); }

Fence::~Fence() {
    if (handle() != vk::Fence())
        wait();
}

FenceBuilder::FenceBuilder(const Device &device)
    : m_device{device} {}

Fence FenceBuilder::build() && {
    const auto info = vk::FenceCreateInfo();

    auto [result, fence] = m_device.handle().createFenceUnique(info);

    return Fence{m_device, std::move(fence)};
}
} // namespace vw
