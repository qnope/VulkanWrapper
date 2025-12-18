#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
class Fence : public ObjectWithUniqueHandle<vk::UniqueFence> {
    friend class FenceBuilder;
    friend class Queue;

  public:
    void wait() const;
    void reset() const;

    Fence(const Fence &) = delete;
    Fence(Fence &&) noexcept = default;
    Fence &operator=(const Fence &) = delete;
    Fence &operator=(Fence &&) noexcept = default;
    ~Fence();

  private:
    Fence(vk::Device device, vk::UniqueFence fence);

    vk::Device m_device;
};

class FenceBuilder {
    friend class Queue;

  public:
    FenceBuilder(std::shared_ptr<const Device> device);

    FenceBuilder &&signaled() &&;

    Fence build() &&;

  private:
    FenceBuilder(vk::Device device);

    vk::Device m_device;
    vk::FenceCreateFlags m_flags{};
};
} // namespace vw
