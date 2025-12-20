#pragma once

#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include <array>
#include <glm/glm.hpp>
#include <random>

// Maximum number of AO samples supported (must match shader)
constexpr int AO_MAX_SAMPLES = 1024;

// UBO structure for AO samples (matches shader layout)
// Each sample contains (xi1, xi2) for cosine-weighted hemisphere sampling
struct AOSamplesUBO {
    // Using vec4 for std140 alignment, only xy are used
    std::array<glm::vec4, AO_MAX_SAMPLES> samples;
};

// Generate random samples for cosine-weighted hemisphere sampling
inline AOSamplesUBO generate_ao_samples() {
    AOSamplesUBO ubo{};

    std::random_device engine;
    std::mt19937 rng(engine());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int i = 0; i < AO_MAX_SAMPLES; ++i) {
        ubo.samples[i] = glm::vec4(dist(rng), dist(rng), 0.0f, 0.0f);
    }

    return ubo;
}

enum class AOPassSlot {
    Output // Single accumulation buffer (no ping-pong needed with blending)
};

/**
 * @brief Functional Ambient Occlusion pass with lazy image allocation
 *
 * This pass lazily allocates its output image on first execute() call.
 * Images are cached by (width, height, frame_index) and reused on subsequent
 * calls.
 */
class AmbientOcclusionPass : public vw::ScreenSpacePass<AOPassSlot> {
  public:
    struct PushConstants {
        float aoRadius;
        int32_t sampleIndex; // Which sample to use (0 to AO_MAX_SAMPLES-1)
    };

    AmbientOcclusionPass(
        std::shared_ptr<vw::Device> device,
        std::shared_ptr<vw::Allocator> allocator,
        vk::AccelerationStructureKHR tlas,
        vk::Format output_format = vk::Format::eR32G32B32A32Sfloat)
        : ScreenSpacePass(device, allocator)
        , m_tlas(tlas)
        , m_output_format(output_format)
        , m_sampler(create_default_sampler())
        , m_ao_samples_buffer(
              vw::create_buffer<AOSamplesUBO, true, vw::UniformBufferUsage>(
                  *m_allocator, 1))
        , m_descriptor_pool(create_descriptor_pool()) {
        // Initialize AO samples buffer
        auto samples = generate_ao_samples();
        m_ao_samples_buffer.write(samples, 0);
    }

    /**
     * @brief Execute the ambient occlusion pass with progressive accumulation
     *
     * Uses hardware blending for temporal accumulation: each frame computes
     * 1 sample and the GPU blends it with the accumulated history.
     * The longer the view stays static, the more accurate the AO becomes.
     *
     * @param cmd Command buffer to record into
     * @param tracker Resource tracker for barrier management (must persist
     *        across frames to maintain state continuity)
     * @param width Render width
     * @param height Render height
     * @param depth_view Depth buffer view (for depth testing)
     * @param position Position G-Buffer view
     * @param normal Normal G-Buffer view
     * @param tangent Tangent G-Buffer view
     * @param bitangent Bitangent G-Buffer view
     * @param ao_radius AO sampling radius
     * @return AO image view
     */
    std::shared_ptr<const vw::ImageView> execute(
        vk::CommandBuffer cmd, vw::Barrier::ResourceTracker &tracker,
        vw::Width width, vw::Height height,
        std::shared_ptr<const vw::ImageView> depth_view,
        std::shared_ptr<const vw::ImageView> position,
        std::shared_ptr<const vw::ImageView> normal,
        std::shared_ptr<const vw::ImageView> tangent,
        std::shared_ptr<const vw::ImageView> bitangent, float ao_radius) {

        // Use fixed frame_index=0 so the image is shared across all swapchain
        // frames. This is required for progressive accumulation to work
        // correctly.
        constexpr size_t ao_frame_index = 0;

        // Single accumulation buffer (no ping-pong needed with hardware
        // blending)
        const auto &output = get_or_create_image(
            AOPassSlot::Output, width, height, ao_frame_index, m_output_format,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferSrc);

        vk::Extent2D extent{static_cast<uint32_t>(width),
                            static_cast<uint32_t>(height)};

        // Create descriptor set with current input images
        vw::DescriptorAllocator descriptor_allocator;
        descriptor_allocator.add_combined_image(
            0, vw::CombinedImage(position, m_sampler),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eShaderRead);
        descriptor_allocator.add_combined_image(
            1, vw::CombinedImage(normal, m_sampler),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eShaderRead);
        descriptor_allocator.add_combined_image(
            2, vw::CombinedImage(tangent, m_sampler),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eShaderRead);
        descriptor_allocator.add_combined_image(
            3, vw::CombinedImage(bitangent, m_sampler),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eShaderRead);
        descriptor_allocator.add_acceleration_structure(
            4, m_tlas, vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eAccelerationStructureReadKHR);
        descriptor_allocator.add_uniform_buffer(
            5, m_ao_samples_buffer.handle(), 0, sizeof(AOSamplesUBO),
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eUniformRead);

        auto descriptor_set =
            m_descriptor_pool.allocate_set(descriptor_allocator);

        // Request resource states for barriers
        for (const auto &resource : descriptor_set.resources()) {
            tracker.request(resource);
        }

        // Output image needs to be in color attachment layout
        tracker.request(vw::Barrier::ImageState{
            .image = output.image->handle(),
            .subresourceRange = output.view->subresource_range(),
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .access = vk::AccessFlagBits2::eColorAttachmentWrite |
                      vk::AccessFlagBits2::eColorAttachmentRead});

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

        // First frame: clear to white (AO = 1.0 = no occlusion)
        // Subsequent frames: load existing content for blending
        bool is_first_frame = (m_total_frame_count == 0);
        vk::RenderingAttachmentInfo color_attachment =
            vk::RenderingAttachmentInfo()
                .setImageView(output.view->handle())
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setLoadOp(is_first_frame ? vk::AttachmentLoadOp::eClear
                                          : vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setClearValue(vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f));

        // Setup depth attachment (read-only for depth testing)
        vk::RenderingAttachmentInfo depth_attachment =
            vk::RenderingAttachmentInfo()
                .setImageView(depth_view->handle())
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eNone);

        // Push constants
        PushConstants constants{.aoRadius = ao_radius,
                                .sampleIndex = static_cast<int32_t>(
                                    m_total_frame_count % AO_MAX_SAMPLES)};

        // Set blend constants for progressive accumulation
        // blend_factor = 1/(frameCount+1) gives equal weight to all samples
        // Frame 0: factor=1.0 (100% new sample)
        // Frame 1: factor=0.5 (average of 2 samples)
        // Frame N: factor=1/(N+1)
        float blend_factor = 1.0f / static_cast<float>(m_total_frame_count + 1);
        std::array<float, 4> blend_constants = {blend_factor, blend_factor,
                                                blend_factor, 1.0f};
        cmd.setBlendConstants(blend_constants.data());

        // Render fullscreen quad with depth testing
        render_fullscreen(cmd, extent, color_attachment, &depth_attachment,
                          *m_pipeline, descriptor_set, &constants,
                          sizeof(constants));

        // Increment frame count AFTER rendering (so first frame is 0)
        m_total_frame_count++;

        return output.view;
    }

    /** @brief Reset progressive accumulation (call on resize) */
    void reset_accumulation() { m_total_frame_count = 0; }

  private:
    vw::DescriptorPool create_descriptor_pool() {
        // Create descriptor layout (no history texture needed with hardware
        // blending)
        m_descriptor_layout =
            vw::DescriptorSetLayoutBuilder(m_device)
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // Position
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // Normal
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // Tangent
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // Bitangent
                .with_acceleration_structure(
                    vk::ShaderStageFlagBits::eFragment) // TLAS
                .with_uniform_buffer(vk::ShaderStageFlagBits::eFragment,
                                     1) // AO samples UBO
                .build();

        // Create pipeline with blending for temporal accumulation
        auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/fullscreen.spv");
        auto fragment_shader = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/post-process/ambient_occlusion.spv");

        std::vector<vk::PushConstantRange> push_constants = {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0,
                                  sizeof(PushConstants))};

        m_pipeline = vw::create_screen_space_pipeline(
            m_device, vertex_shader, fragment_shader, m_descriptor_layout,
            m_output_format, vk::Format::eD32Sfloat, vk::CompareOp::eGreater,
            push_constants, vw::ColorBlendConfig::constant_blend());

        return vw::DescriptorPoolBuilder(m_device, m_descriptor_layout).build();
    }

    vk::AccelerationStructureKHR m_tlas;
    vk::Format m_output_format;

    // Progressive accumulation state
    uint32_t m_total_frame_count = 0;

    // Resources (order matters! m_pipeline must be before m_descriptor_pool)
    std::shared_ptr<const vw::Sampler> m_sampler;
    vw::Buffer<AOSamplesUBO, true, vw::UniformBufferUsage> m_ao_samples_buffer;
    std::shared_ptr<vw::DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<const vw::Pipeline> m_pipeline;
    vw::DescriptorPool m_descriptor_pool;
};
