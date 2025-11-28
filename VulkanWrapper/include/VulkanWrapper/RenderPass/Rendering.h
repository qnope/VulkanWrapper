#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/fwd.h"
#include <memory>
#include <vector>

namespace vw {

class Rendering {
  public:
    struct SubpassInfo {
        std::shared_ptr<Subpass> subpass;
        std::vector<std::shared_ptr<const ImageView>> color_attachments;
        std::shared_ptr<const ImageView> depth_attachment;
    };

    Rendering(std::vector<SubpassInfo> subpasses);

    void execute(vk::CommandBuffer cmd_buffer,
                 Barrier::ResourceTracker &resource_tracker) const;

  private:
    std::vector<SubpassInfo> m_subpasses;
};

class RenderingBuilder {
  public:
    RenderingBuilder();

    RenderingBuilder &&add_subpass(
        std::shared_ptr<Subpass> subpass,
        std::vector<std::shared_ptr<const ImageView>> color_attachments,
        std::shared_ptr<const ImageView> depth_attachment = nullptr) &&;

    Rendering build() &&;

  private:
    std::vector<Rendering::SubpassInfo> m_subpasses;
};

} // namespace vw
