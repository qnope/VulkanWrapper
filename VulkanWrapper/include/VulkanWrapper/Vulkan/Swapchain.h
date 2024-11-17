#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
class Swapchain : public ObjectWithUniqueHandle<vk::UniqueSwapchainKHR> {
  public:
    Swapchain(Device &device, vk::UniqueSwapchainKHR swapchain,
              vk::Format format);

  private:
    Device &m_device;
    vk::Format m_format;
    std::vector<Image> m_images;
    std::vector<ImageView> m_imageViews;
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

} // namespace vw
