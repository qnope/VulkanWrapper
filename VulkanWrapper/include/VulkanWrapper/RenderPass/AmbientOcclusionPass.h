#pragma once

#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Random/NoiseTexture.h"
#include "VulkanWrapper/Random/RandomSamplingBuffer.h"
#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include <filesystem>

namespace vw {

/**
 * @brief Screen-space ambient occlusion pass with progressive
 *        temporal accumulation
 *
 * Uses ray queries in the fragment shader to compute
 * per-pixel ambient occlusion. Each frame computes 1 sample
 * per pixel, and hardware blending with
 * ColorBlendConfig::constant_blend() accumulates results over
 * time for noise-free output.
 *
 * Inputs: Slot::Depth, Slot::Position, Slot::Normal,
 *         Slot::Tangent, Slot::Bitangent
 * Output: Slot::AmbientOcclusion
 */
class AmbientOcclusionPass : public ScreenSpacePass {
  public:
    struct PushConstants {
        float aoRadius;
        int32_t sampleIndex;
    };

    AmbientOcclusionPass(
        std::shared_ptr<Device> device,
        std::shared_ptr<Allocator> allocator,
        const std::filesystem::path &shader_dir,
        vk::AccelerationStructureKHR tlas,
        vk::Format output_format =
            vk::Format::eR32G32B32A32Sfloat,
        vk::Format depth_format =
            vk::Format::eD32Sfloat);

    std::vector<Slot> input_slots() const override;
    std::vector<Slot> output_slots() const override;

    std::string_view name() const override {
        return "AmbientOcclusionPass";
    }

    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker &tracker,
                 Width width, Height height,
                 size_t frame_index) override;

    /// Reset progressive accumulation (call on camera
    /// move or resize)
    void reset_accumulation() override;

    /// Get the current frame count for progressive
    /// accumulation
    uint32_t get_frame_count() const;

    /// Set the AO sampling radius
    void set_ao_radius(float radius);

  private:
    vk::Format m_output_format;
    vk::Format m_depth_format;
    vk::AccelerationStructureKHR m_tlas;

    // Progressive accumulation state
    uint32_t m_frame_count = 0;
    float m_ao_radius = 200.0f;

    // Resources
    std::shared_ptr<const Sampler> m_sampler;
    DualRandomSampleBuffer m_hemisphere_samples;
    std::unique_ptr<NoiseTexture> m_noise_texture;
    std::shared_ptr<DescriptorSetLayout>
        m_descriptor_layout;
    std::shared_ptr<const Pipeline> m_pipeline;
    DescriptorPool m_descriptor_pool;
};

} // namespace vw
