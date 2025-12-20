#include "VulkanWrapper/Vulkan/Swapchain.h"

#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Synchronization/Semaphore.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/PresentQueue.h"

namespace vw {

Swapchain::Swapchain(std::shared_ptr<const Device> device,
                     vk::UniqueSwapchainKHR swapchain, vk::Format format,
                     Width width, Height height)
    : vw::ObjectWithUniqueHandle<vk::UniqueSwapchainKHR>{std::move(swapchain)}
    , m_device{std::move(device)}
    , m_format{format}
    , m_width{width}
    , m_height{height} {
    auto vkImages = m_device->handle().getSwapchainImagesKHR(handle()).value;

    for (auto &vkImage : vkImages) {
        auto &image = m_images.emplace_back(std::make_shared<Image>(
            vkImage, m_width, m_height, Depth(1), MipLevel(1), m_format,
            vk::ImageUsageFlagBits::eColorAttachment, nullptr, nullptr));

        m_image_views.emplace_back(ImageViewBuilder(m_device, image)
                                       .setImageType(vk::ImageViewType::e2D)
                                       .build());
    }
}

Width Swapchain::width() const noexcept { return m_width; }

Height Swapchain::height() const noexcept { return m_height; }

vk::Format Swapchain::format() const noexcept { return m_format; }

std::span<const std::shared_ptr<const Image>>
Swapchain::images() const noexcept {
    return m_images;
}

std::span<const std::shared_ptr<const ImageView>>
Swapchain::image_views() const noexcept {
    return m_image_views;
}

int Swapchain::number_images() const noexcept { return m_images.size(); }

uint64_t Swapchain::acquire_next_image(const Semaphore &semaphore) const {
    auto [result, index] = m_device->handle().acquireNextImageKHR(
        handle(), UINT64_MAX, semaphore.handle());

    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
        throw SwapchainException(result, "Swapchain needs recreation");
    }

    if (result != vk::Result::eSuccess) {
        throw VulkanException(result, "Failed to acquire swapchain image");
    }

    return index;
}

void Swapchain::present(uint32_t index, const Semaphore &waitSemaphore) const {
    m_device->presentQueue().present(*this, index, waitSemaphore);
}

SwapchainBuilder::SwapchainBuilder(std::shared_ptr<const Device> device,
                                   vk::SurfaceKHR surface, Width width,
                                   Height height) noexcept
    : m_device{std::move(device)}
    , m_width{width}
    , m_height{height} {
    m_info.setSurface(surface)
        .setImageExtent(vk::Extent2D(uint32_t(width), uint32_t(height)))
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageFormat(vk::Format::eB8G8R8A8Srgb)
        .setPresentMode(vk::PresentModeKHR::eFifo)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                       vk::ImageUsageFlagBits::eTransferDst |
                       vk::ImageUsageFlagBits::eTransferSrc)
        .setImageArrayLayers(1)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setClipped(1U)
        .setMinImageCount(3);
}

SwapchainBuilder &SwapchainBuilder::with_old_swapchain(vk::SwapchainKHR old) {
    m_info.setOldSwapchain(old);
    return *this;
}

Swapchain SwapchainBuilder::build() {
    auto swapchain =
        check_vk(m_device->handle().createSwapchainKHRUnique(m_info),
                 "Failed to create swapchain");

    return Swapchain{m_device, std::move(swapchain), m_info.imageFormat,
                     m_width, m_height};
}

} // namespace vw
