#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
namespace vw {

using RenderPassCreationException =
    TaggedException<struct RenderPassCreationTag>;

class RenderPass : public ObjectWithUniqueHandle<vk::UniqueRenderPass> {
    friend class RenderPassBuilder;

  public:
    RenderPass(vk::UniqueRenderPass render_pass,
               std::vector<Attachment> attachments);

    [[nodiscard]] const std::vector<vk::ClearValue> &
    clear_values() const noexcept;

  private:
    std::vector<Attachment> m_attachments;
    std::vector<vk::ClearValue> m_clear_values;
};

class RenderPassBuilder {
  public:
    RenderPassBuilder(const Device &device);
    RenderPassBuilder &&add_subpass(Subpass subpass) &&;
    RenderPass build() &&;

  private:
    [[nodiscard]] std::vector<Attachment> create_attachments() const noexcept;

    const Device *m_device;
    std::vector<vw::Subpass> m_subpasses;
};
} // namespace vw
