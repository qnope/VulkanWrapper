#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
using FramebufferCreationException =
    TaggedException<struct FramebufferCreationTag>;

class Framebuffer : public ObjectWithUniqueHandle<vk::UniqueFramebuffer> {
    using ObjectWithUniqueHandle<vk::UniqueFramebuffer>::ObjectWithUniqueHandle;
    friend class FramebufferBuilder;
};

class FramebufferBuilder {
  public:
    FramebufferBuilder(const RenderPass &renderPass, uint32_t width,
                       uint32_t height);

    FramebufferBuilder addAttachment(const ImageView &imageView) &&;

    Framebuffer build(Device &device) &&;

  private:
    vk::RenderPass m_renderPass;
    uint32_t m_width;
    uint32_t m_height;
    std::vector<vk::ImageView> m_attachments;
};

} // namespace vw
