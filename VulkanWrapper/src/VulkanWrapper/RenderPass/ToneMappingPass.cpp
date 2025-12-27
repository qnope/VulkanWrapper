#include "VulkanWrapper/RenderPass/ToneMappingPass.h"

namespace vw {

ToneMappingPass::ToneMappingPass(std::shared_ptr<Device> device,
                                 std::shared_ptr<Allocator> allocator,
                                 const std::filesystem::path &shader_dir,
                                 vk::Format output_format)
    : ScreenSpacePass(device, allocator)
    , m_output_format(output_format)
    , m_sampler(create_default_sampler())
    , m_descriptor_pool(create_descriptor_pool(compile_shaders(shader_dir))) {}

void ToneMappingPass::execute(vk::CommandBuffer cmd,
                              Barrier::ResourceTracker &tracker,
                              std::shared_ptr<const ImageView> output_view,
                              std::shared_ptr<const ImageView> hdr_view,
                              ToneMappingOperator tone_operator, float exposure,
                              float white_point, float luminance_scale) {

    vk::Extent2D extent = output_view->image()->extent2D();

    // Create descriptor set with HDR input image
    DescriptorAllocator descriptor_allocator;
    descriptor_allocator.add_combined_image(
        0, CombinedImage(hdr_view, m_sampler),
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
                            .luminance_scale = luminance_scale};

    // Render fullscreen quad (no depth testing needed)
    render_fullscreen(cmd, extent, color_attachment, nullptr, *m_pipeline,
                      descriptor_set, &constants, sizeof(constants));
}

std::shared_ptr<const ImageView>
ToneMappingPass::execute(vk::CommandBuffer cmd,
                         Barrier::ResourceTracker &tracker, Width width,
                         Height height, size_t frame_index,
                         std::shared_ptr<const ImageView> hdr_view,
                         ToneMappingOperator tone_operator, float exposure,
                         float white_point, float luminance_scale) {

    // Get or create cached output image
    auto &cached = get_or_create_image(
        ToneMappingPassSlot{}, width, height, frame_index, m_output_format,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled);

    execute(cmd, tracker, cached.view, hdr_view, tone_operator, exposure,
            white_point, luminance_scale);

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
    // Create descriptor layout for HDR input texture
    m_descriptor_layout =
        DescriptorSetLayoutBuilder(m_device)
            .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                 1) // HDR buffer
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

} // namespace vw
