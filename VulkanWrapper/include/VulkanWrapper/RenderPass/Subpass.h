#pragma once

#include "VulkanWrapper/RenderPass/Attachment.h"

namespace vw {

struct Subpass {
    std::map<Attachment, vk::ImageLayout> colorReferences;
};

class SubpassBuilder {
  public:
    SubpassBuilder addColorAttachment(Attachment attachment,
                                      vk::ImageLayout layout) &&;
    Subpass build() &&;

  private:
    std::map<Attachment, vk::ImageLayout> m_colorReferences;
};
} // namespace vw
