#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
using FramebufferCreationException =
    TaggedException<struct FramebufferCreationTag>;

class Framebuffer : public ObjectWithUniqueHandle<vk::UniqueFramebuffer> {
    friend class FramebufferBuilder;
    Framebuffer(vk::UniqueFramebuffer framebuffer, vw::Width width,
                vw::Height height) noexcept;

  public:
    [[nodiscard]] Width width() const noexcept;
    [[nodiscard]] Height height() const noexcept;
    [[nodiscard]] vk::Extent2D extent2D() const noexcept;

  private:
    Width m_width;
    Height m_height;
};

class FramebufferBuilder {
  public:
    FramebufferBuilder(const Device &device, const RenderPass &renderPass,
                       Width width, Height height);

    FramebufferBuilder &&
    add_attachment(const std::shared_ptr<ImageView> &imageView) &&;

    Framebuffer build() &&;

  private:
    const Device *m_device;
    vk::RenderPass m_renderPass;
    Width m_width;
    Height m_height;
    std::vector<std::shared_ptr<ImageView>> m_attachments;
};

} // namespace vw
