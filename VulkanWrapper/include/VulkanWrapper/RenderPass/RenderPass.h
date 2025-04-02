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
               std::vector<vk::AttachmentDescription2> attachments,
               std::vector<vk::ClearValue> clear_values,
               std::vector<std::unique_ptr<Subpass>> subpasses);

    [[nodiscard]] const std::vector<vk::ClearValue> &
    clear_values() const noexcept;

    void execute(vk::CommandBuffer cmd_buffer, const Framebuffer &framebuffer,
                 std::span<const vk::DescriptorSet> first_descriptor_sets);

  private:
    std::vector<vk::AttachmentDescription2> m_attachments;
    std::vector<vk::ClearValue> m_clear_values;
    std::vector<std::unique_ptr<Subpass>> m_subpasses;
};

class RenderPassBuilder {
  public:
    RenderPassBuilder(const Device &device);
    [[nodiscard]] RenderPassBuilder &&
    add_attachment(vk::AttachmentDescription2 attachment,
                   vk::ClearValue clear_value) &&;
    [[nodiscard]] RenderPassBuilder &&
    add_subpass(std::unique_ptr<Subpass> subpass) &&;
    [[nodiscard]] RenderPass build() &&;

  private:
    const Device *m_device;
    std::vector<vk::AttachmentDescription2> m_attachments;
    std::vector<vk::ClearValue> m_clear_values;
    std::vector<std::unique_ptr<vw::Subpass>> m_subpasses;
};
} // namespace vw
