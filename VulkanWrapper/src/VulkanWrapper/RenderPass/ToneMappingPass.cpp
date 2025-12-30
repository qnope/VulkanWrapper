#include "VulkanWrapper/RenderPass/ToneMappingPass.h"

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Vulkan/Queue.h"

namespace vw {

ToneMappingPass::ToneMappingPass(std::shared_ptr<Device> device,
                                 std::shared_ptr<Allocator> allocator,
                                 const std::filesystem::path &shader_dir,
                                 vk::Format output_format)
    : ScreenSpacePass(device, allocator)
    , m_output_format(output_format)
    , m_sampler(create_default_sampler())
    , m_descriptor_pool(create_descriptor_pool(compile_shaders(shader_dir))) {
    create_black_fallback_image();
}

void ToneMappingPass::execute(vk::CommandBuffer cmd,
                              Barrier::ResourceTracker &tracker,
                              std::shared_ptr<const ImageView> output_view,
                              std::shared_ptr<const ImageView> hdr_view,
                              std::shared_ptr<const ImageView> indirect_view,
                              float indirect_intensity,
                              ToneMappingOperator tone_operator, float exposure,
                              float white_point, float luminance_scale) {

    vk::Extent2D extent = output_view->image()->extent2D();

    // Use fallback black image if no indirect light provided
    auto effective_indirect_view =
        indirect_view ? indirect_view : m_black_image_view;

    // Create descriptor set with HDR input images
    DescriptorAllocator descriptor_allocator;
    descriptor_allocator.add_combined_image(
        0, CombinedImage(hdr_view, m_sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    descriptor_allocator.add_combined_image(
        1, CombinedImage(effective_indirect_view, m_sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);

    auto descriptor_set = m_descriptor_pool.allocate_set(descriptor_allocator);

    // Request resource states for barriers
    for (const auto &resource : descriptor_set.resources()) {
        tracker.request(resource);
    }

    // Output image needs to be in color attachment layout
    tracker.request(Barrier::ImageState{
        .image = output_view->image()->handle(),
        .subresourceRange = output_view->subresource_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite});

    // Flush barriers
    tracker.flush(cmd);

    // Setup color attachment (clear not needed, we overwrite all pixels)
    vk::RenderingAttachmentInfo color_attachment =
        vk::RenderingAttachmentInfo()
            .setImageView(output_view->handle())
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStoreOp(vk::AttachmentStoreOp::eStore);

    // Push constants
    PushConstants constants{.exposure = exposure,
                            .operator_id = static_cast<int32_t>(tone_operator),
                            .white_point = white_point,
                            .luminance_scale = luminance_scale,
                            .indirect_intensity = indirect_intensity};

    // Render fullscreen quad (no depth testing needed)
    render_fullscreen(cmd, extent, color_attachment, nullptr, *m_pipeline,
                      descriptor_set, &constants, sizeof(constants));
}

std::shared_ptr<const ImageView>
ToneMappingPass::execute(vk::CommandBuffer cmd,
                         Barrier::ResourceTracker &tracker, Width width,
                         Height height, size_t frame_index,
                         std::shared_ptr<const ImageView> hdr_view,
                         std::shared_ptr<const ImageView> indirect_view,
                         float indirect_intensity,
                         ToneMappingOperator tone_operator, float exposure,
                         float white_point, float luminance_scale) {

    // Get or create cached output image
    auto &cached = get_or_create_image(
        ToneMappingPassSlot{}, width, height, frame_index, m_output_format,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc);

    execute(cmd, tracker, cached.view, hdr_view, indirect_view,
            indirect_intensity, tone_operator, exposure, white_point,
            luminance_scale);

    return cached.view;
}

ToneMappingPass::CompiledShaders
ToneMappingPass::compile_shaders(const std::filesystem::path &shader_dir) {
    ShaderCompiler compiler;
    return CompiledShaders{
        .vertex = compiler.compile_file_to_module(m_device,
                                                  shader_dir / "fullscreen.vert"),
        .fragment = compiler.compile_file_to_module(m_device,
                                                    shader_dir / "tonemap.frag")};
}

DescriptorPool
ToneMappingPass::create_descriptor_pool(CompiledShaders shaders) {
    // Create descriptor layout for HDR input textures (direct + indirect)
    m_descriptor_layout =
        DescriptorSetLayoutBuilder(m_device)
            .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                 1) // binding 0: direct light (HDR buffer)
            .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                 1) // binding 1: indirect light
            .build();

    std::vector<vk::PushConstantRange> push_constants = {
        vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0,
                              sizeof(PushConstants))};

    // No depth testing needed for tone mapping
    m_pipeline = create_screen_space_pipeline(
        m_device, shaders.vertex, shaders.fragment, m_descriptor_layout,
        m_output_format, vk::Format::eUndefined, vk::CompareOp::eAlways,
        push_constants);

    return DescriptorPoolBuilder(m_device, m_descriptor_layout).build();
}

void ToneMappingPass::create_black_fallback_image() {
    // Create a 1x1 black image for when indirect_view is not provided
    m_black_image = m_allocator->create_image_2D(
        Width{1}, Height{1}, false, vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);

    m_black_image_view = ImageViewBuilder(m_device, m_black_image)
                             .setImageType(vk::ImageViewType::e2D)
                             .build();

    // Clear the image to black (all zeros) using a transfer command
    // This ensures the image has defined content when sampled
    auto cmd_pool = CommandPoolBuilder(m_device).build();
    auto cmd = cmd_pool.allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Transition from undefined to transfer destination
    vk::ImageMemoryBarrier2 barrier_to_transfer =
        vk::ImageMemoryBarrier2()
            .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eClear)
            .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setImage(m_black_image->handle())
            .setSubresourceRange(m_black_image->full_range());

    cmd.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(
        barrier_to_transfer));

    // Clear the image to black
    vk::ClearColorValue clear_color(0.0f, 0.0f, 0.0f, 0.0f);
    auto subresource_range = m_black_image->full_range();
    cmd.clearColorImage(m_black_image->handle(),
                        vk::ImageLayout::eTransferDstOptimal, clear_color,
                        subresource_range);

    // Transition to shader read optimal
    vk::ImageMemoryBarrier2 barrier_to_read =
        vk::ImageMemoryBarrier2()
            .setSrcStageMask(vk::PipelineStageFlagBits2::eClear)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImage(m_black_image->handle())
            .setSubresourceRange(m_black_image->full_range());

    cmd.pipelineBarrier2(
        vk::DependencyInfo().setImageMemoryBarriers(barrier_to_read));

    std::ignore = cmd.end();
    m_device->graphicsQueue().enqueue_command_buffer(cmd);
    m_device->graphicsQueue().submit({}, {}, {}).wait();
}

} // namespace vw
