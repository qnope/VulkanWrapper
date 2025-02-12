#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
using FramebufferCreationException =
    TaggedException<struct FramebufferCreationTag>;

class Framebuffer : public ObjectWithUniqueHandle<vk::UniqueFramebuffer> {
    friend class FramebufferBuilder;
    Framebuffer(vk::UniqueFramebuffer framebuffer, uint32_t width,
                uint32_t height) noexcept;

  public:
    uint32_t width() const noexcept;
    uint32_t height() const noexcept;

  private:
    uint32_t m_width;
    uint32_t m_height;
};

class FramebufferBuilder {
  public:
    FramebufferBuilder(const Device &device, const RenderPass &renderPass,
                       uint32_t width, uint32_t height);

    FramebufferBuilder &&add_attachment(const ImageView &imageView) &&;

    Framebuffer build() &&;

  private:
    const Device &m_device;
    vk::RenderPass m_renderPass;
    uint32_t m_width;
    uint32_t m_height;
    std::vector<ImageView> m_attachments;
};

} // namespace vw
