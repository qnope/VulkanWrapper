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
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>

// Empty slot enum - SunLightPass doesn't allocate images
enum class SunLightPassSlot {};

/**
 * @brief Functional Sun Light pass (no image allocation)
 *
 * This pass renders sun light contribution additively onto the light buffer.
 * It does not allocate any images - it uses the light buffer from SkyPass as
 * input/output.
 */
class SunLightPass : public vw::ScreenSpacePass<SunLightPassSlot> {
  public:
    struct PushConstants {
        glm::vec4 sunDirection;
        glm::vec4 sunColor;
    };

    SunLightPass(std::shared_ptr<vw::Device> device,
                 std::shared_ptr<vw::Allocator> allocator,
                 vk::AccelerationStructureKHR tlas,
                 vk::Format light_format = vk::Format::eR16G16B16A16Sfloat)
        : ScreenSpacePass(device, allocator)
        , m_tlas(tlas)
        , m_light_format(light_format)
        , m_sampler(create_default_sampler())
        , m_descriptor_pool(create_descriptor_pool()) {}

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
     * @param sun_angle Sun angle in degrees above horizon (90 = zenith)
     */
    void execute(vk::CommandBuffer cmd, vw::Barrier::ResourceTracker &tracker,
                 std::shared_ptr<const vw::ImageView> light_view,
                 std::shared_ptr<const vw::ImageView> depth_view,
                 std::shared_ptr<const vw::ImageView> color_view,
                 std::shared_ptr<const vw::ImageView> position_view,
                 std::shared_ptr<const vw::ImageView> normal_view,
                 float sun_angle) {

        vk::Extent2D extent = light_view->image()->extent2D();

        // Create descriptor set with current input images
        vw::DescriptorAllocator descriptor_allocator;
        descriptor_allocator.add_combined_image(
            0, vw::CombinedImage(color_view, m_sampler),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eShaderRead);
        descriptor_allocator.add_combined_image(
            1, vw::CombinedImage(position_view, m_sampler),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eShaderRead);
        descriptor_allocator.add_combined_image(
            2, vw::CombinedImage(normal_view, m_sampler),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eShaderRead);
        descriptor_allocator.add_acceleration_structure(
            3, m_tlas, vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eAccelerationStructureReadKHR);

        auto descriptor_set =
            m_descriptor_pool.allocate_set(descriptor_allocator);

        // Request resource states for barriers
        for (const auto &resource : descriptor_set.resources()) {
            tracker.request(resource);
        }

        // Light image needs to be in color attachment layout for additive
        // rendering
        tracker.request(vw::Barrier::ImageState{
            .image = light_view->image()->handle(),
            .subresourceRange = light_view->subresource_range(),
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .access = vk::AccessFlagBits2::eColorAttachmentRead |
                      vk::AccessFlagBits2::eColorAttachmentWrite});

        // Depth image for reading
        tracker.request(vw::Barrier::ImageState{
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

        // Push constants
        // Sun direction is FROM sun TO surface (negative of direction to sun)
        float angle_rad = glm::radians(sun_angle);
        PushConstants constants{.sunDirection =
                                    glm::vec4(-std::cos(angle_rad),
                                              -std::sin(angle_rad), 0.0f, 0.0f),
                                .sunColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)};

        // Render fullscreen quad with depth testing
        render_fullscreen(cmd, extent, color_attachment, &depth_attachment,
                          *m_pipeline, descriptor_set, constants);
    }

  private:
    vw::DescriptorPool create_descriptor_pool() {
        // Create descriptor layout
        m_descriptor_layout =
            vw::DescriptorSetLayoutBuilder(m_device)
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // Color
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // Position
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // Normal
                .with_acceleration_structure(
                    vk::ShaderStageFlagBits::eFragment) // TLAS
                .build();

        // Create pipeline
        auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/fullscreen.spv");
        auto fragment_shader = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/post-process/sun_light.spv");

        std::vector<vk::PushConstantRange> push_constants = {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0,
                                  sizeof(PushConstants))};

        m_pipeline = vw::create_screen_space_pipeline(
            m_device, vertex_shader, fragment_shader, m_descriptor_layout,
            m_light_format, vk::Format::eD32Sfloat, vk::CompareOp::eGreater,
            push_constants);

        return vw::DescriptorPoolBuilder(m_device, m_descriptor_layout).build();
    }

    vk::AccelerationStructureKHR m_tlas;
    vk::Format m_light_format;

    // Resources (order matters!)
    std::shared_ptr<const vw::Sampler> m_sampler;
    std::shared_ptr<vw::DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<const vw::Pipeline> m_pipeline;
    vw::DescriptorPool m_descriptor_pool;
};
