#pragma once

#include "3DRenderer/3rd_party.h"
#include "3DRenderer/Core/Vulkan/ObjectWithHandle.h"
#include "3DRenderer/Core/Vulkan/PresentQueue.h"
#include "3DRenderer/Core/fwd.h"

namespace r3d {
class Device : public ObjectWithUniqueHandle<vk::UniqueDevice> {
    friend class DeviceFinder;

  public:
  private:
    Device(vk::UniqueDevice device, std::vector<Queue> queues,
           std::optional<PresentQueue> presentQueue) noexcept;

  private:
    std::vector<Queue> m_queues;
    std::optional<PresentQueue> m_presentQueue;
};

} // namespace r3d
