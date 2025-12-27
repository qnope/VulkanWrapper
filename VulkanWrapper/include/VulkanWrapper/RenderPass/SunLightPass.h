#pragma once

#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Sampler.h"
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

// Empty slot enum - SunLightPass doesn't allocate images
enum class SunLightPassSlot {};

/**
 * @brief Functional Sun Light pass (no image allocation)
 *
 * This pass renders sun light contribution additively onto the light buffer.
 * It does not allocate any images - it uses the light buffer from SkyPass as
 * input/output.
 *
 * Uses ray queries for shadow tracing, requires Vulkan 1.2+ and ray query
 * extension.
 *
 * Shaders are compiled at runtime from GLSL source files using ShaderCompiler.
 */
class SunLightPass : public ScreenSpacePass<SunLightPassSlot> {
  public:
    // Use SkyParametersGPU directly as push constants
    using PushConstants = SkyParametersGPU;

    /**
     * @brief Construct a SunLightPass with shaders loaded from files
     *
     * @param device Vulkan device
     * @param allocator Memory allocator
     * @param shader_dir Path to the shader directory containing fullscreen.vert
     *                   and sun_light.frag
     * @param tlas Top-level acceleration structure for shadow rays
     * @param light_format Output light buffer format
     */
    SunLightPass(std::shared_ptr<Device> device,
                 std::shared_ptr<Allocator> allocator,
                 const std::filesystem::path &shader_dir,
                 vk::AccelerationStructureKHR tlas,
                 vk::Format light_format = vk::Format::eR16G16B16A16Sfloat);

    /**
     * @brief Execute the sun light rendering pass
     *
     * Renders sun light contribution additively onto the light buffer.
     * The light_view is both input (for blending) and output.
     *
     * @param cmd Command buffer to record into
     * @param tracker Resource tracker for barrier management
     * @param light_view Light buffer to render into (from SkyPass)
     * @param depth_view Depth buffer view (for depth testing)
     * @param color_view Color G-Buffer view
     * @param position_view Position G-Buffer view
     * @param normal_view Normal G-Buffer view
     * @param ao_view Ambient occlusion buffer view
     * @param sky_params Sky and sun parameters
     */
    void execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
                 std::shared_ptr<const ImageView> light_view,
                 std::shared_ptr<const ImageView> depth_view,
                 std::shared_ptr<const ImageView> color_view,
                 std::shared_ptr<const ImageView> position_view,
                 std::shared_ptr<const ImageView> normal_view,
                 std::shared_ptr<const ImageView> ao_view,
                 const SkyParameters &sky_params);

  private:
    DescriptorPool
    create_descriptor_pool(const std::filesystem::path &shader_dir);

    vk::AccelerationStructureKHR m_tlas;
    vk::Format m_light_format;

    // Resources (order matters!)
    std::shared_ptr<const Sampler> m_sampler;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<const Pipeline> m_pipeline;
    DescriptorPool m_descriptor_pool;
};

} // namespace vw
