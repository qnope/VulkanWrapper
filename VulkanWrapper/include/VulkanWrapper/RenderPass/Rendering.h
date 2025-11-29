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
    Rendering(std::vector<std::shared_ptr<Subpass>> subpasses);

    void execute(vk::CommandBuffer cmd_buffer,
                 Barrier::ResourceTracker &resource_tracker) const;

  private:
    std::vector<std::shared_ptr<Subpass>> m_subpasses;
};

class RenderingBuilder {
  public:
    RenderingBuilder();

    RenderingBuilder &&add_subpass(std::shared_ptr<Subpass> subpass) &&;

    Rendering build() &&;

  private:
    std::vector<std::shared_ptr<Subpass>> m_subpasses;
};

} // namespace vw
