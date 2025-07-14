#include "VulkanWrapper/Image/Framebuffer.h"

#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"
#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
Framebuffer::Framebuffer(
    vk::UniqueFramebuffer framebuffer, Width width, Height height,
    std::vector<std::shared_ptr<const ImageView>> views) noexcept
    : ObjectWithUniqueHandle<vk::UniqueFramebuffer>{std::move(framebuffer)}
    , m_width{width}
    , m_height{height}
    , m_image_views{std::move(views)} {}

Width Framebuffer::width() const noexcept { return m_width; }

Height Framebuffer::height() const noexcept { return m_height; }

vk::Extent2D Framebuffer::extent2D() const noexcept {
    return {uint32_t(m_width), uint32_t(m_height)};
}

std::shared_ptr<const ImageView>
Framebuffer::image_view(int index) const noexcept {
    return m_image_views[index];
}

FramebufferBuilder::FramebufferBuilder(const Device &device,
                                       const IRenderPass &renderPass,
                                       Width width, Height height)
    : m_device{&device}
    , m_renderPass{renderPass.handle()}
    , m_width{width}
    , m_height{height} {}

FramebufferBuilder &&FramebufferBuilder::add_attachment(
    const std::shared_ptr<const ImageView> &imageView) && {
    m_attachments.push_back(imageView);
    return std::move(*this);
}

Framebuffer FramebufferBuilder::build() && {
    const auto attachments = m_attachments | to_handle | to<std::vector>;

    const auto info = vk::FramebufferCreateInfo()
                          .setWidth(uint32_t(m_width))
                          .setHeight(uint32_t(m_height))
                          .setLayers(1)
                          .setRenderPass(m_renderPass)
                          .setAttachments(attachments);

    auto [result, framebuffer] =
        m_device->handle().createFramebufferUnique(info);
    if (result != vk::Result::eSuccess) {
        throw FramebufferCreationException{std::source_location::current()};
    }
    return Framebuffer{std::move(framebuffer), m_width, m_height,
                       std::move(m_attachments)};
}

} // namespace vw
