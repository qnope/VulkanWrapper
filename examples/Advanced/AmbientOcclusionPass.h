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
constexpr int AO_MAX_SAMPLES = 256;

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

enum class AOPassSlot { Output };

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
        int32_t numSamples;
    };

    AmbientOcclusionPass(std::shared_ptr<vw::Device> device,
                         std::shared_ptr<vw::Allocator> allocator,
                         vk::AccelerationStructureKHR tlas,
                         vk::Format output_format = vk::Format::eR8G8B8A8Unorm)
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
        m_ao_samples_buffer.copy(samples, 0);
    }

    /**
     * @brief Execute the ambient occlusion pass
     *
     * @param cmd Command buffer to record into
     * @param tracker Resource tracker for barrier management
     * @param width Render width
     * @param height Render height
     * @param frame_index Frame index for multi-buffering
     * @param depth_view Depth buffer view (for depth testing)
     * @param position Position G-Buffer view
     * @param normal Normal G-Buffer view
     * @param tangent Tangent G-Buffer view
     * @param bitangent Bitangent G-Buffer view
     * @param num_samples Number of AO samples (clamped to 1-256)
     * @param ao_radius AO sampling radius
     * @return The output AO image view
     */
    std::shared_ptr<const vw::ImageView>
    execute(vk::CommandBuffer cmd, vw::Barrier::ResourceTracker &tracker,
            vw::Width width, vw::Height height, size_t frame_index,
            std::shared_ptr<const vw::ImageView> depth_view,
            std::shared_ptr<const vw::ImageView> position,
            std::shared_ptr<const vw::ImageView> normal,
            std::shared_ptr<const vw::ImageView> tangent,
            std::shared_ptr<const vw::ImageView> bitangent, int32_t num_samples,
            float ao_radius) {

        // Lazy allocation of output image
        const auto &output = get_or_create_image(
            AOPassSlot::Output, width, height, frame_index, m_output_format,
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
                .setImageView(output.view->handle())
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

        // Setup depth attachment (read-only for depth testing)
        vk::RenderingAttachmentInfo depth_attachment =
            vk::RenderingAttachmentInfo()
                .setImageView(depth_view->handle())
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eNone);

        // Push constants
        PushConstants constants{.aoRadius = ao_radius,
                                .numSamples =
                                    std::clamp(num_samples, 1, AO_MAX_SAMPLES)};

        // Render fullscreen quad with depth testing
        render_fullscreen(cmd, extent, color_attachment, &depth_attachment,
                          *m_pipeline, descriptor_set, constants);

        return output.view;
    }

  private:
    vw::DescriptorPool create_descriptor_pool() {
        // Create descriptor layout
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

        // Create pipeline
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
            push_constants);

        return vw::DescriptorPoolBuilder(m_device, m_descriptor_layout).build();
    }

    vk::AccelerationStructureKHR m_tlas;
    vk::Format m_output_format;

    // Resources (order matters! m_pipeline must be before m_descriptor_pool)
    std::shared_ptr<const vw::Sampler> m_sampler;
    vw::Buffer<AOSamplesUBO, true, vw::UniformBufferUsage> m_ao_samples_buffer;
    std::shared_ptr<vw::DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<const vw::Pipeline> m_pipeline;
    vw::DescriptorPool m_descriptor_pool;
};
