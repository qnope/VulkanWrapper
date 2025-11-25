#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/IdentifierTag.h"
#include <vulkan/vulkan.hpp>
#include <span>

namespace vw {

struct SubpassDependencyMask {
    vk::PipelineStageFlags stage = vk::PipelineStageFlagBits::eNone;
    vk::AccessFlags access = vk::AccessFlagBits::eNone;
};

using SubpassTag = IdentifierTag<struct SubpassIdentifierTag>;

template <typename T> SubpassTag create_subpass_tag() {
    return SubpassTag{typeid(T)};
}

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
};

} // namespace vw
