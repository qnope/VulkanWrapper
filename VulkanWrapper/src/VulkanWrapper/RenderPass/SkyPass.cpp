#include "VulkanWrapper/RenderPass/SkyPass.h"

namespace vw {

SkyPass::SkyPass(std::shared_ptr<Device> device,
                 std::shared_ptr<Allocator> allocator,
                 const std::filesystem::path &shader_dir,
                 vk::Format light_format, vk::Format depth_format)
    : ScreenSpacePass(device, allocator)
    , m_light_format(light_format)
    , m_depth_format(depth_format)
    , m_pipeline(create_pipeline(shader_dir)) {}

std::shared_ptr<const ImageView>
SkyPass::execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
                 Width width, Height height, size_t frame_index,
                 std::shared_ptr<const ImageView> depth_view,
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
    tracker.request(Barrier::ImageState{
        .image = light.image->handle(),
        .subresourceRange = light.view->subresource_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite});

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
                      *m_pipeline, std::nullopt, &constants, sizeof(constants));

    return light.view;
}

std::shared_ptr<const Pipeline>
SkyPass::create_pipeline(const std::filesystem::path &shader_dir) {
    ShaderCompiler compiler;
    compiler.add_include_path(shader_dir / "include");

    auto vertex_shader = compiler.compile_file_to_module(
        m_device, shader_dir / "fullscreen.vert");
    auto fragment_shader =
        compiler.compile_file_to_module(m_device, shader_dir / "sky.frag");

    std::vector<vk::PushConstantRange> push_constants = {
        vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0,
                              sizeof(PushConstants))};

    return create_screen_space_pipeline(
        m_device, vertex_shader, fragment_shader, nullptr, m_light_format,
        m_depth_format, vk::CompareOp::eEqual, push_constants);
}

} // namespace vw
