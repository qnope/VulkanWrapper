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

enum class SkyPassSlot { Light };

/**
 * @brief Functional Sky pass with lazy image allocation
 *
 * This pass lazily allocates its light output image on first execute() call.
 * Images are cached by (width, height, frame_index) and reused on subsequent
 * calls. The sky is rendered where depth == 1.0 (far plane).
 *
 * Shaders are compiled at runtime from GLSL source files using ShaderCompiler.
 */
class SkyPass : public ScreenSpacePass<SkyPassSlot> {
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
     * @param shader_dir Path to the shader directory containing fullscreen.vert
     *                   and sky.frag
     * @param light_format Output light buffer format
     * @param depth_format Depth buffer format
     */
    SkyPass(std::shared_ptr<Device> device,
            std::shared_ptr<Allocator> allocator,
            const std::filesystem::path &shader_dir,
            vk::Format light_format = vk::Format::eR32G32B32A32Sfloat,
            vk::Format depth_format = vk::Format::eD32Sfloat);

    /**
     * @brief Execute the sky rendering pass
     *
     * @param cmd Command buffer to record into
     * @param tracker Resource tracker for barrier management
     * @param width Render width
     * @param height Render height
     * @param frame_index Frame index for multi-buffering
     * @param depth_view Depth buffer view (for depth testing at far plane)
     * @param sky_params Sky and sun parameters
     * @param inverse_view_proj Inverse view-projection matrix
     * @return The output light image view
     */
    std::shared_ptr<const ImageView>
    execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
            Width width, Height height, size_t frame_index,
            std::shared_ptr<const ImageView> depth_view,
            const SkyParameters &sky_params,
            const glm::mat4 &inverse_view_proj);

  private:
    std::shared_ptr<const Pipeline>
    create_pipeline(const std::filesystem::path &shader_dir);

    vk::Format m_light_format;
    vk::Format m_depth_format;
    std::shared_ptr<const Pipeline> m_pipeline;
};

} // namespace vw
