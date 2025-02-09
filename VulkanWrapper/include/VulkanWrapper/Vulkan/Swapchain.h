#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
class Swapchain : public ObjectWithUniqueHandle<vk::UniqueSwapchainKHR> {
  public:
    Swapchain(const Device &device, vk::UniqueSwapchainKHR swapchain,
              vk::Format format, int width, int height);

    int width() const noexcept;
    int height() const noexcept;
    vk::Format format() const noexcept;

    std::vector<Image> images() const noexcept;

    uint64_t acquire_next_image(const Semaphore &semaphore) const noexcept;

  private:
    const Device &m_device;
    vk::Format m_format;
    std::vector<Image> m_images;
    int m_width;
    int m_height;
};

class SwapchainBuilder {
  public:
    SwapchainBuilder(const Device &device, vk::SurfaceKHR surface, int width,
                     int height) noexcept;
    Swapchain build() &&;

  private:
    const Device &m_device;
    int m_width;
    int m_height;

    vk::SwapchainCreateInfoKHR m_info;
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eMailbox;
};

} // namespace vw
