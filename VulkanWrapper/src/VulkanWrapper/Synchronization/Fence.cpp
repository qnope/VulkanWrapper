#include "VulkanWrapper/Synchronization/Fence.h"

#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

Fence::Fence(vk::Device device, vk::UniqueFence fence)
    : ObjectWithUniqueHandle<vk::UniqueFence>{std::move(fence)}
    , m_device{std::move(device)} {}

void Fence::wait() const {
    std::ignore = m_device.waitForFences(handle(), 1U,
                                         std::numeric_limits<uint64_t>::max());
}

void Fence::reset() const { std::ignore = m_device.resetFences(handle()); }

Fence::~Fence() {
    if (handle() != vk::Fence()) {
        wait();
    }
}

FenceBuilder::FenceBuilder(vk::Device device)
    : m_device{std::move(device)} {}

Fence FenceBuilder::build() && {
    const auto info = vk::FenceCreateInfo();

    auto [result, fence] = m_device.createFenceUnique(info);

    return Fence{m_device, std::move(fence)};
}
} // namespace vw
