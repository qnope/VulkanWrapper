#pragma once

#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Model/Material/BindlessMaterialManager.h"
#include "VulkanWrapper/RayTracing/GeometryReference.h"
#include "VulkanWrapper/RayTracing/RayTracingPipeline.h"
#include "VulkanWrapper/RayTracing/ShaderBindingTable.h"
#include "VulkanWrapper/RayTracing/TopLevelAccelerationStructure.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"
#include "VulkanWrapper/RenderPass/SkyParameters.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include <filesystem>

namespace vw {

/**
 * @brief Push constants for IndirectLightPass
 *
 * Contains sky atmosphere parameters plus frame control for
 * accumulation. Shared between raygen, miss, and closest hit
 * shaders.
 */
struct IndirectLightPushConstants {
    SkyParametersGPU sky; // 96 bytes
    uint32_t frame_count; // 4 bytes
    uint32_t width;       // 4 bytes
    uint32_t height;      // 4 bytes
}; // 108 bytes total

static_assert(sizeof(IndirectLightPushConstants) <= 128,
              "IndirectLightPushConstants must fit in push "
              "constant limit");

/**
 * @brief Ray Tracing Indirect Light pass with progressive
 *        accumulation
 *
 * This pass computes indirect sky lighting using a ray tracing
 * pipeline. It reads precomputed ray directions from the GBuffer
 * indirect_ray attachment and traces rays from each surface point.
 *
 * Inputs: Slot::Position, Slot::Normal, Slot::Albedo,
 *         Slot::AmbientOcclusion, Slot::IndirectRay
 * Outputs: Slot::IndirectLight
 *
 * The pass uses progressive accumulation: each frame computes
 * 1 sample per pixel and blends it with the accumulated history
 * using imageLoad/imageStore.
 */
class IndirectLightPass : public RenderPass {
  public:
    /**
     * @brief Construct an IndirectLightPass with shaders loaded
     *        from files
     */
    IndirectLightPass(
        std::shared_ptr<Device> device,
        std::shared_ptr<Allocator> allocator,
        const std::filesystem::path &shader_dir,
        const rt::as::TopLevelAccelerationStructure &tlas,
        const rt::GeometryReferenceBuffer &geometry_buffer,
        Model::Material::BindlessMaterialManager
            &material_manager,
        vk::Format output_format =
            vk::Format::eR32G32B32A32Sfloat);

    // -- Slot introspection --
    std::vector<Slot> input_slots() const override {
        return {Slot::Position, Slot::Normal, Slot::Albedo,
                Slot::AmbientOcclusion, Slot::IndirectRay};
    }
    std::vector<Slot> output_slots() const override {
        return {Slot::IndirectLight};
    }

    // -- Unified execute --
    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker &tracker,
                 Width width, Height height,
                 size_t frame_index) override;

    std::string_view name() const override {
        return "IndirectLightPass";
    }

    // -- Setters for non-slot data --
    void set_sky_parameters(const SkyParameters &params) {
        m_sky_params = params;
    }

    /**
     * @brief Reset progressive accumulation
     *
     * Call this when the camera moves or any parameter changes
     * that would invalidate the accumulated result.
     */
    void reset_accumulation() override { m_frame_count = 0; }

    /**
     * @brief Get the current frame count for progressive
     *        accumulation
     */
    uint32_t get_frame_count() const { return m_frame_count; }

  private:
    void create_pipeline_and_sbt(
        const std::filesystem::path &shader_dir);

    const rt::as::TopLevelAccelerationStructure *m_tlas;
    const rt::GeometryReferenceBuffer *m_geometry_buffer;
    Model::Material::BindlessMaterialManager *m_material_manager;
    vk::Format m_output_format;

    // Progressive accumulation state
    uint32_t m_frame_count = 0;

    // Stored parameters
    SkyParameters m_sky_params =
        SkyParameters::create_earth_sun(45.0f);

    // Resources (order matters for destruction!)
    std::shared_ptr<const Sampler> m_sampler;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_layout;
    std::unique_ptr<rt::RayTracingPipeline> m_pipeline;
    std::unique_ptr<rt::ShaderBindingTable> m_sbt;
    DescriptorPool m_descriptor_pool;

    // Texture descriptor resources for per-material hit shaders
    std::shared_ptr<DescriptorSetLayout>
        m_texture_descriptor_layout;
    DescriptorPool m_texture_descriptor_pool;
};

} // namespace vw
