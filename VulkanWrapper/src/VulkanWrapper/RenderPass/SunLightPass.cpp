#include "VulkanWrapper/RenderPass/SunLightPass.h"

namespace vw {

SunLightPass::SunLightPass(std::shared_ptr<Device> device,
                           std::shared_ptr<Allocator> allocator,
                           const std::filesystem::path &shader_dir,
                           vk::AccelerationStructureKHR tlas,
                           vk::Format light_format)
    : ScreenSpacePass(device, allocator)
    , m_tlas(tlas)
    , m_light_format(light_format)
    , m_sampler(create_default_sampler())
    , m_descriptor_pool(create_descriptor_pool(shader_dir)) {}

void SunLightPass::execute(vk::CommandBuffer cmd,
                           Barrier::ResourceTracker &tracker,
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

    auto descriptor_set = m_descriptor_pool.allocate_set(descriptor_allocator);

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

DescriptorPool
SunLightPass::create_descriptor_pool(const std::filesystem::path &shader_dir) {
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
    compiler.add_include_path(shader_dir / "include");

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

} // namespace vw
