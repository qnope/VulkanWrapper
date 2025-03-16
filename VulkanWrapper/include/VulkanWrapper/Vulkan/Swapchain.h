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

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] vk::Format format() const noexcept;

    [[nodiscard]] std::span<const Image> images() const noexcept;

    [[nodiscard]] uint64_t
    acquire_next_image(const Semaphore &semaphore) const noexcept;

  private:
    const Device *m_device;
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
    const Device *m_device;
    int m_width;
    int m_height;

    vk::SwapchainCreateInfoKHR m_info;
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eMailbox;
};

} // namespace vw
