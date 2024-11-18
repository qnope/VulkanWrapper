#include "VulkanWrapper/Vulkan/Swapchain.h"

#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

Swapchain::Swapchain(Device &device, vk::UniqueSwapchainKHR swapchain,
                     vk::Format format, int width, int height)
    : vw::ObjectWithUniqueHandle<vk::UniqueSwapchainKHR>{std::move(swapchain)}
    , m_device{device}
    , m_format{format}
    , m_width{width}
    , m_height{height} {
    auto vkImages = m_device.handle().getSwapchainImagesKHR(handle()).value;

    for (auto &vkImage : vkImages) {
        auto &image = m_images.emplace_back(vkImage, m_format);

        auto imageView = ImageViewBuilder(device, image).build();
        m_imageViews.push_back(std::move(imageView));
    }
}

int Swapchain::width() const noexcept { return m_width; }

int Swapchain::height() const noexcept { return m_height; }

SwapchainBuilder::SwapchainBuilder(Device &device, vk::SurfaceKHR surface,
                                   int width, int height) noexcept
    : m_device{device}
    , m_width{width}
    , m_height{height} {
    m_info.setSurface(surface)
        .setImageExtent(vk::Extent2D(width, height))
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageFormat(vk::Format::eB8G8R8A8Srgb)
        .setPresentMode(vk::PresentModeKHR::eMailbox)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setImageArrayLayers(1)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setClipped(true)
        .setMinImageCount(3);
}

Swapchain SwapchainBuilder::build() && {

    auto result = m_device.handle().createSwapchainKHRUnique(m_info);

    if (result.result != vk::Result::eSuccess)
        throw 0;

    return Swapchain{m_device, std::move(result.value), m_info.imageFormat,
                     m_width, m_height};
}

} // namespace vw
