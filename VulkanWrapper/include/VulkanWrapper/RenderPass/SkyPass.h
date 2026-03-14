#pragma once

#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include "VulkanWrapper/RenderPass/SkyParameters.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include <filesystem>
#include <glm/glm.hpp>

namespace vw {

/**
 * @brief Functional Sky pass with lazy image allocation
 *
 * This pass lazily allocates its light output image on first
 * execute() call. Images are cached by (width, height, frame_index)
 * and reused on subsequent calls. The sky is rendered where
 * depth == 1.0 (far plane).
 *
 * Shaders are compiled at runtime from GLSL source files using
 * ShaderCompiler.
 *
 * Inputs: Slot::Depth (depth buffer for depth testing at far plane)
 * Outputs: Slot::Sky (HDR sky radiance)
 */
class SkyPass : public ScreenSpacePass {
  public:
    // Combined push constants: SkyParametersGPU + inverseViewProj
    struct PushConstants {
        SkyParametersGPU sky;      // Full sky parameters
        glm::mat4 inverseViewProj; // inverse(projection * view)
    };

    /**
     * @brief Construct a SkyPass with shaders loaded from files
     *
     * @param device Vulkan device
     * @param allocator Memory allocator
     * @param shader_dir Path to the shader directory containing
     *                   fullscreen.vert and sky.frag
     * @param light_format Output light buffer format
     * @param depth_format Depth buffer format
     */
    SkyPass(std::shared_ptr<Device> device,
            std::shared_ptr<Allocator> allocator,
            const std::filesystem::path &shader_dir,
            vk::Format light_format = vk::Format::eR32G32B32A32Sfloat,
            vk::Format depth_format = vk::Format::eD32Sfloat);

    // -- Slot introspection --
    std::vector<Slot> input_slots() const override {
        return {Slot::Depth};
    }
    std::vector<Slot> output_slots() const override {
        return {Slot::Sky};
    }

    // -- Execute (unified signature) --
    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker &tracker, Width width,
                 Height height, size_t frame_index) override;

    std::string_view name() const override { return "SkyPass"; }

    // -- Setters for non-slot data --
    void set_sky_parameters(const SkyParameters &params) {
        m_sky_params = params;
    }
    void set_inverse_view_proj(const glm::mat4 &inverse_vp) {
        m_inverse_view_proj = inverse_vp;
    }

  private:
    std::shared_ptr<const Pipeline>
    create_pipeline(const std::filesystem::path &shader_dir);

    vk::Format m_light_format;
    vk::Format m_depth_format;
    std::shared_ptr<const Pipeline> m_pipeline;

    // Stored parameters used during execute()
    SkyParameters m_sky_params =
        SkyParameters::create_earth_sun(45.0f);
    glm::mat4 m_inverse_view_proj = glm::mat4(1.0f);
};

} // namespace vw
