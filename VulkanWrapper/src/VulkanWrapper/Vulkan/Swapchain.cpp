#include "VulkanWrapper/Vulkan/Swapchain.h"

#include "VulkanWrapper/Synchronization/Semaphore.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

Swapchain::Swapchain(const Device &device, vk::UniqueSwapchainKHR swapchain,
                     vk::Format format, Width width, Height height)
    : vw::ObjectWithUniqueHandle<vk::UniqueSwapchainKHR>{std::move(swapchain)}
    , m_device{&device}
    , m_format{format}
    , m_width{width}
    , m_height{height} {
    auto vkImages = m_device->handle().getSwapchainImagesKHR(handle()).value;

    for (auto &vkImage : vkImages) {
        auto &image = m_images.emplace_back(std::make_shared<Image>(
            vkImage, m_width, m_height, Depth(1), MipLevel(1), m_format,
            vk::ImageUsageFlagBits::eColorAttachment, nullptr, nullptr));
    }
}

Width Swapchain::width() const noexcept { return m_width; }

Height Swapchain::height() const noexcept { return m_height; }

vk::Format Swapchain::format() const noexcept { return m_format; }

std::span<const std::shared_ptr<const Image>>
Swapchain::images() const noexcept {
    return m_images;
}

int Swapchain::number_images() const noexcept { return m_images.size(); }

uint64_t
Swapchain::acquire_next_image(const Semaphore &semaphore) const noexcept {
    return m_device->handle()
        .acquireNextImageKHR(handle(), UINT64_MAX, semaphore.handle())
        .value;
}

SwapchainBuilder::SwapchainBuilder(const Device &device, vk::SurfaceKHR surface,
                                   Width width, Height height) noexcept
    : m_device{&device}
    , m_width{width}
    , m_height{height} {
    m_info.setSurface(surface)
        .setImageExtent(vk::Extent2D(uint32_t(width), uint32_t(height)))
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageFormat(vk::Format::eB8G8R8A8Srgb)
        .setPresentMode(vk::PresentModeKHR::eFifo)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                       vk::ImageUsageFlagBits::eTransferDst)
        .setImageArrayLayers(1)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setClipped(1U)
        .setMinImageCount(3);
}

Swapchain SwapchainBuilder::build() && {

    auto result = m_device->handle().createSwapchainKHRUnique(m_info);

    if (result.result != vk::Result::eSuccess) {
        throw 0;
    }

    return Swapchain{*m_device, std::move(result.value), m_info.imageFormat,
                     m_width, m_height};
}

} // namespace vw
