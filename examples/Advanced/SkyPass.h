#pragma once

#include "SkyParameters.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include <glm/glm.hpp>

enum class SkyPassSlot { Light };

/**
 * @brief Functional Sky pass with lazy image allocation
 *
 * This pass lazily allocates its light output image on first execute() call.
 * Images are cached by (width, height, frame_index) and reused on subsequent
 * calls. The sky is rendered where depth == 1.0 (far plane).
 */
class SkyPass : public vw::ScreenSpacePass<SkyPassSlot> {
  public:
    // Combined push constants: SkyParametersGPU + inverseViewProj
    struct PushConstants {
        SkyParametersGPU sky;      // Full sky parameters
        glm::mat4 inverseViewProj; // inverse(projection * view)
    };

    SkyPass(std::shared_ptr<vw::Device> device,
            std::shared_ptr<vw::Allocator> allocator,
            vk::Format light_format = vk::Format::eR32G32B32A32Sfloat,
            vk::Format depth_format = vk::Format::eD32Sfloat)
        : ScreenSpacePass(device, allocator)
        , m_light_format(light_format)
        , m_depth_format(depth_format)
        , m_pipeline(create_pipeline()) {}

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
    std::shared_ptr<const vw::ImageView>
    execute(vk::CommandBuffer cmd, vw::Barrier::ResourceTracker &tracker,
            vw::Width width, vw::Height height, size_t frame_index,
            std::shared_ptr<const vw::ImageView> depth_view,
            const SkyParameters &sky_params,
            const glm::mat4 &inverse_view_proj) {

        // Lazy allocation of light image
        const auto &light = get_or_create_image(
            SkyPassSlot::Light, width, height, frame_index, m_light_format,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferSrc);

        vk::Extent2D extent{static_cast<uint32_t>(width),
                            static_cast<uint32_t>(height)};

        // Light image needs to be in color attachment layout
        tracker.request(vw::Barrier::ImageState{
            .image = light.image->handle(),
            .subresourceRange = light.view->subresource_range(),
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .access = vk::AccessFlagBits2::eColorAttachmentWrite});

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

        // Setup color attachment (clear)
        vk::RenderingAttachmentInfo color_attachment =
            vk::RenderingAttachmentInfo()
                .setImageView(light.view->handle())
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

        // Setup depth attachment (read-only)
        vk::RenderingAttachmentInfo depth_attachment =
            vk::RenderingAttachmentInfo()
                .setImageView(depth_view->handle())
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eNone);

        // Use full SkyParametersGPU as push constants
        PushConstants constants{.sky = sky_params.to_gpu(),
                                .inverseViewProj = inverse_view_proj};

        // Render fullscreen quad with depth testing (sky only where depth
        // == 1.0)
        render_fullscreen(cmd, extent, color_attachment, &depth_attachment,
                          *m_pipeline, std::nullopt, &constants,
                          sizeof(constants));

        return light.view;
    }

  private:
    std::shared_ptr<const vw::Pipeline> create_pipeline() {
        auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/fullscreen.spv");
        auto fragment_shader = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/post-process/sky.spv");

        std::vector<vk::PushConstantRange> push_constants = {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0,
                                  sizeof(PushConstants))};

        return vw::create_screen_space_pipeline(
            m_device, vertex_shader, fragment_shader, nullptr, m_light_format,
            m_depth_format, vk::CompareOp::eEqual, push_constants);
    }

    vk::Format m_light_format;
    vk::Format m_depth_format;
    std::shared_ptr<const vw::Pipeline> m_pipeline;
};
