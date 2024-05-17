#include "3DRenderer/Core/Vulkan/Device.h"
#include "3DRenderer/Core/Vulkan/Queue.h"

namespace r3d {

Device::Device(vk::UniqueDevice device, std::vector<Queue> queues,
               std::optional<PresentQueue> presentQueue) noexcept
    : ObjectWithUniqueHandle<vk::UniqueDevice>{std::move(device)}
    , m_queues{std::move(queues)}
    , m_presentQueue{std::move(presentQueue)} {}

} // namespace r3d
