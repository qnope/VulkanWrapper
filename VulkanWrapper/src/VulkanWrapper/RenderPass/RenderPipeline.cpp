#include "VulkanWrapper/RenderPass/RenderPipeline.h"

#include <map>
#include <set>
#include <string>

namespace vw {

namespace {

std::string slot_to_string(Slot slot) {
    switch (slot) {
    case Slot::Depth:
        return "Depth";
    case Slot::Albedo:
        return "Albedo";
    case Slot::Normal:
        return "Normal";
    case Slot::Tangent:
        return "Tangent";
    case Slot::Bitangent:
        return "Bitangent";
    case Slot::Position:
        return "Position";
    case Slot::DirectLight:
        return "DirectLight";
    case Slot::IndirectRay:
        return "IndirectRay";
    case Slot::AmbientOcclusion:
        return "AmbientOcclusion";
    case Slot::Sky:
        return "Sky";
    case Slot::IndirectLight:
        return "IndirectLight";
    case Slot::ToneMapped:
        return "ToneMapped";
    default:
        return "Slot(" +
               std::to_string(static_cast<int>(slot)) + ")";
    }
}

} // namespace

RenderPipeline::ValidationResult
RenderPipeline::validate() const {
    std::vector<std::string> errors;
    std::set<Slot> available;

    for (size_t i = 0; i < m_passes.size(); ++i) {
        const auto &pass = *m_passes[i];

        for (auto slot : pass.input_slots()) {
            if (!available.contains(slot)) {
                errors.push_back(
                    "Pass " + std::to_string(i) + " (" +
                    std::string(pass.name()) +
                    ") requires Slot::" +
                    slot_to_string(slot) +
                    " but no preceding pass produces it");
            }
        }

        for (auto slot : pass.output_slots()) {
            available.insert(slot);
        }
    }

    return {errors.empty(), std::move(errors)};
}

void RenderPipeline::execute(vk::CommandBuffer cmd,
                             Barrier::ResourceTracker &tracker,
                             Width width, Height height,
                             size_t frame_index) {
    std::map<Slot, CachedImage> slot_outputs;

    for (auto &pass : m_passes) {
        // Wire inputs from preceding passes' outputs
        for (auto slot : pass->input_slots()) {
            auto it = slot_outputs.find(slot);
            if (it != slot_outputs.end()) {
                pass->set_input(slot, it->second);
            }
        }

        pass->execute(cmd, tracker, width, height, frame_index);

        // Collect outputs
        for (auto &[slot, cached] : pass->result_images()) {
            slot_outputs.insert_or_assign(slot,
                                          std::move(cached));
        }
    }
}

void RenderPipeline::reset_accumulation() {
    for (auto &pass : m_passes) {
        pass->reset_accumulation();
    }
}

RenderPass &RenderPipeline::pass(size_t index) {
    return *m_passes[index];
}

const RenderPass &RenderPipeline::pass(size_t index) const {
    return *m_passes[index];
}

size_t RenderPipeline::pass_count() const {
    return m_passes.size();
}

} // namespace vw
