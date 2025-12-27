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
                 vk::Format light_format = vk::Format::eR16G16B16A16Sfloat)
        : ScreenSpacePass(device, allocator)
        , m_tlas(tlas)
        , m_light_format(light_format)
        , m_sampler(create_default_sampler())
        , m_descriptor_pool(create_descriptor_pool(shader_dir)) {}

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
                 const SkyParameters &sky_params) {

        vk::Extent2D extent = light_view->image()->extent2D();

        // Create descriptor set with current input images
        DescriptorAllocator descriptor_allocator;
        descriptor_allocator.add_combined_image(
            0, CombinedImage(color_view, m_sampler),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eShaderRead);
        descriptor_allocator.add_combined_image(
            1, CombinedImage(position_view, m_sampler),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eShaderRead);
        descriptor_allocator.add_combined_image(
            2, CombinedImage(normal_view, m_sampler),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eShaderRead);
        descriptor_allocator.add_acceleration_structure(
            3, m_tlas, vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eAccelerationStructureReadKHR);
        descriptor_allocator.add_combined_image(
            4, CombinedImage(ao_view, m_sampler),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eShaderRead);

        auto descriptor_set =
            m_descriptor_pool.allocate_set(descriptor_allocator);

        // Request resource states for barriers
        for (const auto &resource : descriptor_set.resources()) {
            tracker.request(resource);
        }

        // Light image needs to be in color attachment layout for additive
        // rendering
        tracker.request(Barrier::ImageState{
            .image = light_view->image()->handle(),
            .subresourceRange = light_view->subresource_range(),
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .access = vk::AccessFlagBits2::eColorAttachmentRead |
                      vk::AccessFlagBits2::eColorAttachmentWrite});

        // Depth image for reading
        tracker.request(Barrier::ImageState{
            .image = depth_view->image()->handle(),
            .subresourceRange = depth_view->subresource_range(),
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                     vk::PipelineStageFlagBits2::eLateFragmentTests,
            .access = vk::AccessFlagBits2::eDepthStencilAttachmentRead});

        // Flush barriers
        tracker.flush(cmd);

        // Setup color attachment with additive blending (load existing content)
        vk::RenderingAttachmentInfo color_attachment =
            vk::RenderingAttachmentInfo()
                .setImageView(light_view->handle())
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eStore);

        // Setup depth attachment (read-only for depth testing)
        vk::RenderingAttachmentInfo depth_attachment =
            vk::RenderingAttachmentInfo()
                .setImageView(depth_view->handle())
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eNone);

        // Use full SkyParametersGPU as push constants
        PushConstants constants = sky_params.to_gpu();

        // Render fullscreen quad with depth testing
        render_fullscreen(cmd, extent, color_attachment, &depth_attachment,
                          *m_pipeline, descriptor_set, &constants,
                          sizeof(constants));
    }

  private:
    DescriptorPool create_descriptor_pool(const std::filesystem::path &shader_dir) {
        // Create descriptor layout
        m_descriptor_layout =
            DescriptorSetLayoutBuilder(m_device)
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // Color
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // Position
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // Normal
                .with_acceleration_structure(
                    vk::ShaderStageFlagBits::eFragment) // TLAS
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // AO
                .build();

        // Compile shaders with Vulkan 1.2 for ray query support
        ShaderCompiler compiler;
        compiler.set_target_vulkan_version(VK_API_VERSION_1_2);

        auto vertex_shader = compiler.compile_file_to_module(
            m_device, shader_dir / "fullscreen.vert");
        auto fragment_shader = compiler.compile_file_to_module(
            m_device, shader_dir / "sun_light.frag");

        std::vector<vk::PushConstantRange> push_constants = {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0,
                                  sizeof(PushConstants))};

        m_pipeline = create_screen_space_pipeline(
            m_device, vertex_shader, fragment_shader, m_descriptor_layout,
            m_light_format, vk::Format::eD32Sfloat, vk::CompareOp::eGreater,
            push_constants);

        return DescriptorPoolBuilder(m_device, m_descriptor_layout).build();
    }

    vk::AccelerationStructureKHR m_tlas;
    vk::Format m_light_format;

    // Resources (order matters!)
    std::shared_ptr<const Sampler> m_sampler;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<const Pipeline> m_pipeline;
    DescriptorPool m_descriptor_pool;
};

} // namespace vw
