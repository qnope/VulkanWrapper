#include "VulkanWrapper/Image/Framebuffer.h"

#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"
#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
Framebuffer::Framebuffer(vk::UniqueFramebuffer framebuffer, uint32_t width,
                         uint32_t height) noexcept
    : ObjectWithUniqueHandle<vk::UniqueFramebuffer>{std::move(framebuffer)}
    , m_width{width}
    , m_height{height} {}

uint32_t Framebuffer::width() const noexcept { return m_width; }

uint32_t Framebuffer::height() const noexcept { return m_height; }

FramebufferBuilder::FramebufferBuilder(const Device &device,
                                       const RenderPass &renderPass,
                                       uint32_t width, uint32_t height)
    : m_device{device}
    , m_renderPass{renderPass.handle()}
    , m_width{width}
    , m_height{height} {}

FramebufferBuilder &&
FramebufferBuilder::add_attachment(const ImageView &imageView) && {
    m_attachments.push_back(imageView);
    return std::move(*this);
}

Framebuffer FramebufferBuilder::build() && {
    const auto attachments =
        m_attachments |
        std::views::transform([](auto &&x) { return x.handle(); }) |
        to<std::vector>;

    const auto info = vk::FramebufferCreateInfo()
                          .setWidth(m_width)
                          .setHeight(m_height)
                          .setLayers(1)
                          .setRenderPass(m_renderPass)
                          .setAttachments(attachments);

    auto [result, framebuffer] =
        m_device.handle().createFramebufferUnique(info);
    if (result != vk::Result::eSuccess)
        throw FramebufferCreationException{std::source_location::current()};
    return Framebuffer{std::move(framebuffer), m_width, m_height};
}

} // namespace vw
