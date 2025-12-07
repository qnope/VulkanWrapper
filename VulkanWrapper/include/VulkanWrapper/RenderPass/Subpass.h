#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Utils/IdentifierTag.h"
#include <span>
#include <vulkan/vulkan.hpp>
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Image/ImageView.h"
#include <optional>

namespace vw {

class Subpass {
  public:
    Subpass() noexcept = default;

    Subpass(const Subpass &) = delete;
    Subpass(Subpass &&) = delete;
    Subpass &operator=(Subpass &&) = delete;
    Subpass &operator=(const Subpass &) = delete;

    virtual ~Subpass() = default;

    [[nodiscard]] virtual vk::PipelineBindPoint
    pipeline_bind_point() const noexcept;

    virtual void execute(vk::CommandBuffer cmd_buffer) const noexcept = 0;

    struct AttachmentInfo {
        std::vector<vk::RenderingAttachmentInfo> color;
        std::optional<vk::RenderingAttachmentInfo> depth;
        vk::Rect2D render_area;
    };

    virtual AttachmentInfo attachment_information() const = 0;

    virtual std::vector<vw::Barrier::ResourceState> resource_states() const = 0;
};

} // namespace vw
