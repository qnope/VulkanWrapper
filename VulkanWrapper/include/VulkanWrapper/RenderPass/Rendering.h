#pragma once

#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/fwd.h"
#include <memory>
#include <vector>

namespace vw {

class Rendering {
  public:
    struct SubpassInfo {
        std::unique_ptr<Subpass> subpass;
        std::vector<std::shared_ptr<const ImageView>> color_attachments;
        std::shared_ptr<const ImageView> depth_attachment;
    };

    Rendering(std::vector<SubpassInfo> subpasses);

    void execute(vk::CommandBuffer cmd_buffer) const;

  private:
    std::vector<SubpassInfo> m_subpasses;
};

class RenderingBuilder {
  public:
    RenderingBuilder();

    RenderingBuilder &&add_subpass(
        std::unique_ptr<Subpass> subpass,
        std::vector<std::shared_ptr<const ImageView>> color_attachments,
        std::shared_ptr<const ImageView> depth_attachment = nullptr) &&;

    Rendering build() &&;

  private:
    std::vector<Rendering::SubpassInfo> m_subpasses;
};

} // namespace vw
