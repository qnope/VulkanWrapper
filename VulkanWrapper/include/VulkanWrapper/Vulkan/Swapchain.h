#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

enum class AcquireResult {
    Success,
    OutOfDate,
    Suboptimal
};

struct AcquireImageResult {
    uint32_t imageIndex;
    AcquireResult result;

    [[nodiscard]] bool needs_recreation() const noexcept {
        return result == AcquireResult::OutOfDate;
    }

    [[nodiscard]] bool is_suboptimal() const noexcept {
        return result == AcquireResult::Suboptimal;
    }
};

class Swapchain : public ObjectWithUniqueHandle<vk::UniqueSwapchainKHR> {
  public:
    Swapchain(std::shared_ptr<const Device> device,
              vk::UniqueSwapchainKHR swapchain,
              vk::Format format, Width width, Height height);

    [[nodiscard]] Width width() const noexcept;
    [[nodiscard]] Height height() const noexcept;
    [[nodiscard]] vk::Extent2D extent() const noexcept;
    [[nodiscard]] vk::Format format() const noexcept;

    [[nodiscard]] std::span<const std::shared_ptr<const Image>>
    images() const noexcept;

    [[nodiscard]] int number_images() const noexcept;

    [[nodiscard]] AcquireImageResult
    acquire_next_image(const Semaphore &semaphore) const noexcept;

  private:
    std::shared_ptr<const Device> m_device;
    vk::Format m_format;
    std::vector<std::shared_ptr<const Image>> m_images;
    Width m_width;
    Height m_height;
};

class SwapchainBuilder {
  public:
    SwapchainBuilder(std::shared_ptr<const Device> device, vk::SurfaceKHR surface,
                     Width width, Height height) noexcept;

    SwapchainBuilder &setOldSwapchain(vk::SwapchainKHR oldSwapchain) noexcept;

    Swapchain build() &&;

  private:
    std::shared_ptr<const Device> m_device;
    Width m_width;
    Height m_height;

    vk::SwapchainCreateInfoKHR m_info;
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eMailbox;
};

} // namespace vw
