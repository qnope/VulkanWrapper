#pragma once

#include "VulkanWrapper/RenderPass/RenderPass.h"
#include <concepts>
#include <memory>
#include <string>
#include <vector>

namespace vw {

class RenderPipeline {
  public:
    RenderPipeline() = default;

    // Add a pass. Returns reference for configuration.
    // Order matters: passes execute in insertion order.
    template <std::derived_from<RenderPass> T>
    T &add(std::unique_ptr<T> pass) {
        auto &ref = *pass;
        m_passes.push_back(std::move(pass));
        return ref;
    }

    // Validate slot dependencies
    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;
    };
    ValidationResult validate() const;

    // Execute all passes in order, wiring outputs to inputs
    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker &tracker,
                 Width width, Height height,
                 size_t frame_index);

    // Reset progressive accumulation in all passes
    void reset_accumulation();

    // Access passes by index
    RenderPass &pass(size_t index);
    const RenderPass &pass(size_t index) const;
    size_t pass_count() const;

  private:
    std::vector<std::unique_ptr<RenderPass>> m_passes;
};

} // namespace vw
