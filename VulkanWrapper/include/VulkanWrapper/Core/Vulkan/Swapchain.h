#pragma once

#include "VulkanWrapper/Core/fwd.h"
#include "VulkanWrapper/Core/Vulkan/Image.h"
#include "ObjectWithHandle.h"

namespace r3d {
class Swapchain : public ObjectWithUniqueHandle<vk::UniqueSwapchainKHR> {
  public:
    Swapchain(Device &device, vk::UniqueSwapchainKHR swapchain);

  private:
    Device &m_device;
    std::vector<Image> m_images;
};

class SwapchainBuilder {
  public:
    SwapchainBuilder(Device &device, vk::SurfaceKHR surface, int width,
                     int height) noexcept;
    Swapchain build() &&;

  private:
    Device &m_device;
    int m_width;
    int m_height;

    vk::SwapchainCreateInfoKHR m_info;
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eMailbox;
};

} // namespace r3d
