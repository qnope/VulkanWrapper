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
  private:
    using ObjectWithUniqueHandle<vk::UniqueRenderPass>::ObjectWithUniqueHandle;
};

class RenderPassBuilder {
  public:
    RenderPassBuilder(const Device &device);
    RenderPassBuilder &&add_subpass(Subpass subpass) &&;
    RenderPass build() &&;

  private:
    std::vector<Attachment> createAttachments() const noexcept;

  private:
    const Device &m_device;
    std::vector<std::pair<vk::PipelineBindPoint, vw::Subpass>> m_subpasses;
};
} // namespace vw
