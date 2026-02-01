#pragma once

#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Random/NoiseTexture.h"
#include "VulkanWrapper/Random/RandomSamplingBuffer.h"
#include "VulkanWrapper/RayTracing/RayTracingPipeline.h"
#include "VulkanWrapper/RayTracing/ShaderBindingTable.h"
#include "VulkanWrapper/RayTracing/TopLevelAccelerationStructure.h"
#include "VulkanWrapper/RenderPass/SkyParameters.h"
#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include <filesystem>

namespace vw {

enum class IndirectLightPassSlot {
    Output // Single accumulation buffer (storage image for RT)
};

/**
 * @brief Push constants for IndirectLightPass
 *
 * Contains sky atmosphere parameters plus frame control for accumulation.
 * Shared between raygen, miss, and closest hit shaders.
 */
struct IndirectLightPushConstants {
    SkyParametersGPU sky; // 96 bytes
    uint32_t frame_count; // 4 bytes
    uint32_t width;       // 4 bytes
    uint32_t height;      // 4 bytes
}; // 108 bytes total

static_assert(sizeof(IndirectLightPushConstants) <= 128,
              "IndirectLightPushConstants must fit in push constant limit");

/**
 * @brief Ray Tracing Indirect Light pass with progressive accumulation
 *
 * This pass computes indirect sky lighting using a ray tracing pipeline.
 * It traces cosine-weighted rays from each surface point:
 * - Rays that escape to the sky contribute atmospheric radiance (miss shader)
 * - Rays that hit geometry contribute zero (closest hit shader)
 *
 * The pass uses progressive accumulation: each frame computes 1 sample per
 * pixel and blends it with the accumulated history using imageLoad/imageStore.
 * This produces clean results over time while maintaining real-time performance.
 *
 * Output is written to an independent storage image (not additive to light_view).
 *
 * Shaders are compiled at runtime from GLSL source files using ShaderCompiler:
 * - indirect_light.rgen: Ray generation shader
 * - indirect_light.rmiss: Miss shader (computes atmosphere)
 * - indirect_light.rchit: Closest hit shader (returns black)
 */
class IndirectLightPass : public Subpass<IndirectLightPassSlot> {
  public:
    /**
     * @brief Construct an IndirectLightPass with shaders loaded from files
     *
     * @param device Vulkan device
     * @param allocator Memory allocator
     * @param shader_dir Path to the shader directory containing the RT shaders
     * @param tlas Top-level acceleration structure for occlusion rays
     * @param output_format Output buffer format (default: R32G32B32A32Sfloat)
     */
    IndirectLightPass(std::shared_ptr<Device> device,
                      std::shared_ptr<Allocator> allocator,
                      const std::filesystem::path &shader_dir,
                      const rt::as::TopLevelAccelerationStructure &tlas,
                      vk::Format output_format = vk::Format::eR32G32B32A32Sfloat);

    /**
     * @brief Execute the indirect light pass with progressive accumulation
     *
     * Uses imageLoad/imageStore for temporal accumulation: each frame computes
     * 1 sample per pixel and blends it with the accumulated history.
     * The longer the view stays static, the more accurate the lighting becomes.
     *
     * Combines indirect sky lighting with ambient contribution modulated by AO.
     * Uses the same ambient formula as the direct sun lighting pass:
     * ambient = (albedo/PI) * L_sun * solid_angle * 0.05 * ao
     *
     * @param cmd Command buffer to record into
     * @param tracker Resource tracker for barrier management
     * @param width Render width
     * @param height Render height
     * @param position_view Position G-Buffer view
     * @param normal_view Normal G-Buffer view
     * @param albedo_view Albedo G-Buffer view (material color)
     * @param ao_view Ambient occlusion buffer view
     * @param tangent_view Tangent G-Buffer view (for TBN)
     * @param bitangent_view Bitangent G-Buffer view (for TBN)
     * @param sky_params Sky and sun parameters
     * @return The output indirect light image view
     */
    std::shared_ptr<const ImageView>
    execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
            Width width, Height height,
            std::shared_ptr<const ImageView> position_view,
            std::shared_ptr<const ImageView> normal_view,
            std::shared_ptr<const ImageView> albedo_view,
            std::shared_ptr<const ImageView> ao_view,
            std::shared_ptr<const ImageView> tangent_view,
            std::shared_ptr<const ImageView> bitangent_view,
            const SkyParameters &sky_params);

    /**
     * @brief Reset progressive accumulation
     *
     * Call this when the camera moves or any parameter changes that would
     * invalidate the accumulated result.
     */
    void reset_accumulation() { m_frame_count = 0; }

    /**
     * @brief Get the current frame count for progressive accumulation
     */
    uint32_t get_frame_count() const { return m_frame_count; }

  private:
    void create_pipeline_and_sbt(const std::filesystem::path &shader_dir);

    const rt::as::TopLevelAccelerationStructure *m_tlas;
    vk::Format m_output_format;

    // Progressive accumulation state
    uint32_t m_frame_count = 0;

    // Resources (order matters for destruction!)
    std::shared_ptr<const Sampler> m_sampler;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_layout;
    std::unique_ptr<rt::RayTracingPipeline> m_pipeline;
    std::unique_ptr<rt::ShaderBindingTable> m_sbt;
    DescriptorPool m_descriptor_pool;

    // Random sampling resources
    DualRandomSampleBuffer m_samples_buffer;
    std::unique_ptr<NoiseTexture> m_noise_texture;
};

} // namespace vw
