#include "VulkanWrapper/RenderPass/AmbientOcclusionPass.h"

#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Device.h"

#include <array>

namespace vw {

AmbientOcclusionPass::AmbientOcclusionPass(
    std::shared_ptr<Device> device,
    std::shared_ptr<Allocator> allocator,
    const std::filesystem::path &shader_dir,
    vk::AccelerationStructureKHR tlas,
    vk::Format output_format, vk::Format depth_format)
    : ScreenSpacePass(std::move(device),
                      std::move(allocator))
    , m_output_format(output_format)
    , m_depth_format(depth_format)
    , m_tlas(tlas)
    , m_sampler(create_default_sampler())
    , m_hemisphere_samples(
          create_hemisphere_samples_buffer(*m_allocator))
    , m_noise_texture(std::make_unique<NoiseTexture>(
          m_device, m_allocator,
          m_device->graphicsQueue()))
    , m_descriptor_layout(
          DescriptorSetLayoutBuilder(m_device)
              .with_combined_image(
                  vk::ShaderStageFlagBits::eFragment,
                  1) // binding 0: Position
              .with_combined_image(
                  vk::ShaderStageFlagBits::eFragment,
                  1) // binding 1: Normal
              .with_combined_image(
                  vk::ShaderStageFlagBits::eFragment,
                  1) // binding 2: Tangent
              .with_combined_image(
                  vk::ShaderStageFlagBits::eFragment,
                  1) // binding 3: Bitangent
              .with_acceleration_structure(
                  vk::ShaderStageFlagBits::
                      eFragment) // binding 4: TLAS
              .with_storage_buffer(
                  vk::ShaderStageFlagBits::eFragment,
                  1) // binding 5: Xi samples
              .with_combined_image(
                  vk::ShaderStageFlagBits::eFragment,
                  1) // binding 6: Noise texture
              .build())
    , m_pipeline([&] {
        ShaderCompiler compiler;
        compiler.set_target_vulkan_version(
            VK_API_VERSION_1_2);
        compiler.add_include_path(shader_dir / "include");

        auto vertex_shader =
            compiler.compile_file_to_module(
                m_device,
                shader_dir / "fullscreen.vert");
        auto fragment_shader =
            compiler.compile_file_to_module(
                m_device,
                shader_dir / "post-process" /
                    "ambient_occlusion.frag");

        std::vector<vk::PushConstantRange>
            push_constants = {vk::PushConstantRange(
                vk::ShaderStageFlagBits::eFragment, 0,
                sizeof(PushConstants))};

        return create_screen_space_pipeline(
            m_device, vertex_shader, fragment_shader,
            m_descriptor_layout, m_output_format,
            m_depth_format, vk::CompareOp::eGreater,
            push_constants,
            ColorBlendConfig::constant_blend());
    }())
    , m_descriptor_pool(
          DescriptorPoolBuilder(m_device,
                                m_descriptor_layout)
              .build()) {}

std::vector<Slot>
AmbientOcclusionPass::input_slots() const {
    return {Slot::Depth, Slot::Position, Slot::Normal,
            Slot::Tangent, Slot::Bitangent};
}

std::vector<Slot>
AmbientOcclusionPass::output_slots() const {
    return {Slot::AmbientOcclusion};
}

void AmbientOcclusionPass::execute(
    vk::CommandBuffer cmd,
    Barrier::ResourceTracker &tracker, Width width,
    Height height, size_t /*frame_index*/) {

    // Get input views from predecessor passes
    auto depth_view = get_input(Slot::Depth).view;
    auto position_view = get_input(Slot::Position).view;
    auto normal_view = get_input(Slot::Normal).view;
    auto tangent_view = get_input(Slot::Tangent).view;
    auto bitangent_view =
        get_input(Slot::Bitangent).view;

    // Use fixed frame_index=0 for progressive
    // accumulation (single shared buffer)
    constexpr size_t ao_frame_index = 0;

    const auto &output = get_or_create_image(
        Slot::AmbientOcclusion, width, height,
        ao_frame_index, m_output_format,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc);

    vk::Extent2D extent{static_cast<uint32_t>(width),
                        static_cast<uint32_t>(height)};

    // Create descriptor set with current input images
    DescriptorAllocator descriptor_allocator;
    descriptor_allocator.add_combined_image(
        0, CombinedImage(position_view, m_sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    descriptor_allocator.add_combined_image(
        1, CombinedImage(normal_view, m_sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    descriptor_allocator.add_combined_image(
        2, CombinedImage(tangent_view, m_sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    descriptor_allocator.add_combined_image(
        3, CombinedImage(bitangent_view, m_sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    descriptor_allocator.add_acceleration_structure(
        4, m_tlas,
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::
            eAccelerationStructureReadKHR);
    descriptor_allocator.add_storage_buffer(
        5, m_hemisphere_samples.handle(), 0,
        m_hemisphere_samples.size_bytes(),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    descriptor_allocator.add_combined_image(
        6, m_noise_texture->combined_image(),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);

    auto descriptor_set =
        m_descriptor_pool.allocate_set(
            descriptor_allocator);

    // Request resource states for barriers
    for (const auto &resource :
         descriptor_set.resources()) {
        tracker.request(resource);
    }

    // Output image needs to be in color attachment layout
    tracker.request(Barrier::ImageState{
        .image = output.image->handle(),
        .subresourceRange =
            output.view->subresource_range(),
        .layout =
            vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::
            eColorAttachmentOutput,
        .access =
            vk::AccessFlagBits2::eColorAttachmentWrite |
            vk::AccessFlagBits2::
                eColorAttachmentRead});

    // Depth image for reading
    tracker.request(Barrier::ImageState{
        .image = depth_view->image()->handle(),
        .subresourceRange =
            depth_view->subresource_range(),
        .layout = vk::ImageLayout::
            eDepthStencilAttachmentOptimal,
        .stage =
            vk::PipelineStageFlagBits2::
                eEarlyFragmentTests |
            vk::PipelineStageFlagBits2::
                eLateFragmentTests,
        .access = vk::AccessFlagBits2::
            eDepthStencilAttachmentRead});

    // Flush barriers
    tracker.flush(cmd);

    // First frame: clear to white (AO = 1.0 = no
    // occlusion). Subsequent frames: load existing
    // content for blending.
    bool is_first_frame = (m_frame_count == 0);
    vk::RenderingAttachmentInfo color_attachment =
        vk::RenderingAttachmentInfo()
            .setImageView(output.view->handle())
            .setImageLayout(
                vk::ImageLayout::
                    eColorAttachmentOptimal)
            .setLoadOp(
                is_first_frame
                    ? vk::AttachmentLoadOp::eClear
                    : vk::AttachmentLoadOp::eLoad)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearColorValue(
                1.0f, 1.0f, 1.0f, 1.0f));

    // Setup depth attachment (read-only for depth
    // testing)
    vk::RenderingAttachmentInfo depth_attachment =
        vk::RenderingAttachmentInfo()
            .setImageView(depth_view->handle())
            .setImageLayout(
                vk::ImageLayout::
                    eDepthStencilAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eLoad)
            .setStoreOp(vk::AttachmentStoreOp::eNone);

    // Push constants
    PushConstants constants{
        .aoRadius = m_ao_radius,
        .sampleIndex = static_cast<int32_t>(
            m_frame_count % DUAL_SAMPLE_COUNT)};

    // Set blend constants for progressive accumulation
    // blend_factor = 1/(frameCount+1) gives equal weight
    // to all samples
    float blend_factor =
        1.0f / static_cast<float>(m_frame_count + 1);
    std::array<float, 4> blend_constants = {
        blend_factor, blend_factor, blend_factor, 1.0f};
    cmd.setBlendConstants(blend_constants.data());

    // Render fullscreen quad with depth testing
    render_fullscreen(cmd, extent, color_attachment,
                      &depth_attachment, *m_pipeline,
                      descriptor_set, &constants,
                      sizeof(constants));

    // Increment frame count AFTER rendering
    m_frame_count++;
}

void AmbientOcclusionPass::reset_accumulation() {
    m_frame_count = 0;
}

uint32_t
AmbientOcclusionPass::get_frame_count() const {
    return m_frame_count;
}

void AmbientOcclusionPass::set_ao_radius(float radius) {
    m_ao_radius = radius;
}

} // namespace vw
