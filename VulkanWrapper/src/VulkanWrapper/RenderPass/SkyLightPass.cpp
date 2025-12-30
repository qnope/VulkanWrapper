#include "VulkanWrapper/RenderPass/SkyLightPass.h"

#include "VulkanWrapper/Pipeline/PipelineLayout.h"

namespace vw {

SkyLightPass::SkyLightPass(std::shared_ptr<Device> device,
                           std::shared_ptr<Allocator> allocator,
                           const std::filesystem::path &shader_dir,
                           const rt::as::TopLevelAccelerationStructure &tlas,
                           vk::Format output_format)
    : Subpass(device, allocator)
    , m_tlas(&tlas)
    , m_output_format(output_format)
    , m_sampler(SamplerBuilder(m_device).build())
    , m_descriptor_pool(DescriptorPool(m_device, nullptr)) {
    // Create pipeline and SBT
    create_pipeline_and_sbt(shader_dir);
}

void SkyLightPass::create_pipeline_and_sbt(
    const std::filesystem::path &shader_dir) {
    // Create descriptor layout for RT pipeline:
    // binding 0: accelerationStructureEXT (TLAS)
    // binding 1: sampler2D (G-Buffer position)
    // binding 2: sampler2D (G-Buffer normal)
    // binding 3: image2D storage (output - read/write)
    // binding 4: sampler2D (G-Buffer albedo)
    // binding 5: sampler2D (Ambient Occlusion)
    // binding 6: sampler2D (G-Buffer tangent)
    // binding 7: sampler2D (G-Buffer bitangent)
    // Note: Random values are now generated per-pixel using PCG hash in shader
    constexpr auto rt_stages = vk::ShaderStageFlagBits::eRaygenKHR |
                               vk::ShaderStageFlagBits::eMissKHR |
                               vk::ShaderStageFlagBits::eClosestHitKHR;

    m_descriptor_layout =
        DescriptorSetLayoutBuilder(m_device)
            .with_acceleration_structure(rt_stages) // binding 0
            .with_combined_image(rt_stages, 1)      // binding 1: position
            .with_combined_image(rt_stages, 1)      // binding 2: normal
            .with_storage_image(rt_stages, 1)       // binding 3: output
            .with_combined_image(rt_stages, 1)      // binding 4: albedo
            .with_combined_image(rt_stages, 1)      // binding 5: AO
            .with_combined_image(rt_stages, 1)      // binding 6: tangent
            .with_combined_image(rt_stages, 1)      // binding 7: bitangent
            .build();

    // Create pipeline layout with push constants
    auto pipeline_layout =
        PipelineLayoutBuilder(m_device)
            .with_descriptor_set_layout(m_descriptor_layout)
            .with_push_constant_range(
                vk::PushConstantRange(rt_stages, 0, sizeof(SkyLightPushConstants)))
            .build();

    // Compile shaders with Vulkan 1.2 for ray tracing support
    ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_2);
    compiler.add_include_path(shader_dir / "include");

    auto raygen_shader = compiler.compile_file_to_module(
        m_device, shader_dir / "sky_light.rgen");
    auto miss_shader = compiler.compile_file_to_module(
        m_device, shader_dir / "sky_light.rmiss");
    auto closesthit_shader = compiler.compile_file_to_module(
        m_device, shader_dir / "sky_light.rchit");

    // Build RT pipeline
    m_pipeline = std::make_unique<rt::RayTracingPipeline>(
        rt::RayTracingPipelineBuilder(m_device, m_allocator,
                                      std::move(pipeline_layout))
            .set_ray_generation_shader(raygen_shader)
            .add_miss_shader(miss_shader)
            .add_closest_hit_shader(closesthit_shader)
            .build());

    // Create shader binding table
    m_sbt = std::make_unique<rt::ShaderBindingTable>(
        m_allocator, m_pipeline->ray_generation_handle());

    // Add miss shader record
    auto miss_handles = m_pipeline->miss_handles();
    if (!miss_handles.empty()) {
        m_sbt->add_miss_record(miss_handles[0]);
    }

    // Add hit shader record
    auto hit_handles = m_pipeline->closest_hit_handles();
    if (!hit_handles.empty()) {
        m_sbt->add_hit_record(hit_handles[0]);
    }

    // Create descriptor pool
    m_descriptor_pool = DescriptorPoolBuilder(m_device, m_descriptor_layout).build();
}

std::shared_ptr<const ImageView>
SkyLightPass::execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
                      Width width, Height height,
                      std::shared_ptr<const ImageView> position_view,
                      std::shared_ptr<const ImageView> normal_view,
                      std::shared_ptr<const ImageView> albedo_view,
                      std::shared_ptr<const ImageView> ao_view,
                      std::shared_ptr<const ImageView> tangent_view,
                      std::shared_ptr<const ImageView> bitangent_view,
                      const SkyParameters &sky_params) {

    // Use fixed frame_index=0 so the image is shared across all swapchain
    // frames. This is required for progressive accumulation to work correctly.
    constexpr size_t sky_light_frame_index = 0;

    // Single accumulation buffer (storage image for RT)
    const auto &output =
        get_or_create_image(SkyLightPassSlot::Output, width, height,
                            sky_light_frame_index, m_output_format,
                            vk::ImageUsageFlagBits::eStorage |
                                vk::ImageUsageFlagBits::eSampled |
                                vk::ImageUsageFlagBits::eTransferSrc);

    // Create descriptor set with current input images
    DescriptorAllocator descriptor_allocator;

    // binding 0: TLAS
    descriptor_allocator.add_acceleration_structure(
        0, m_tlas->handle(), vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eAccelerationStructureReadKHR);

    // binding 1: Position G-Buffer
    descriptor_allocator.add_combined_image(
        1, CombinedImage(position_view, m_sampler),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eShaderRead);

    // binding 2: Normal G-Buffer
    descriptor_allocator.add_combined_image(
        2, CombinedImage(normal_view, m_sampler),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eShaderRead);

    // binding 3: Output storage image (read/write for accumulation)
    descriptor_allocator.add_storage_image(
        3, *output.view, vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite);

    // binding 4: Albedo G-Buffer
    descriptor_allocator.add_combined_image(
        4, CombinedImage(albedo_view, m_sampler),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eShaderRead);

    // binding 5: Ambient Occlusion
    descriptor_allocator.add_combined_image(
        5, CombinedImage(ao_view, m_sampler),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eShaderRead);

    // binding 6: Tangent G-Buffer
    descriptor_allocator.add_combined_image(
        6, CombinedImage(tangent_view, m_sampler),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eShaderRead);

    // binding 7: Bitangent G-Buffer
    descriptor_allocator.add_combined_image(
        7, CombinedImage(bitangent_view, m_sampler),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eShaderRead);

    auto descriptor_set = m_descriptor_pool.allocate_set(descriptor_allocator);

    // Request resource states for barriers
    for (const auto &resource : descriptor_set.resources()) {
        tracker.request(resource);
    }

    // Output image needs to be in General layout for storage image access
    tracker.request(Barrier::ImageState{
        .image = output.image->handle(),
        .subresourceRange = output.view->subresource_range(),
        .layout = vk::ImageLayout::eGeneral,
        .stage = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .access = vk::AccessFlagBits2::eShaderRead |
                  vk::AccessFlagBits2::eShaderWrite});

    // Flush barriers
    tracker.flush(cmd);

    // Bind RT pipeline
    cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,
                     m_pipeline->handle());

    // Bind descriptor set
    auto descriptor_handle = descriptor_set.handle();
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                           m_pipeline->handle_layout(), 0, 1,
                           &descriptor_handle, 0, nullptr);

    // Push constants
    SkyLightPushConstants constants{.sky = sky_params.to_gpu(),
                                    .frame_count = m_frame_count,
                                    .width = static_cast<uint32_t>(width),
                                    .height = static_cast<uint32_t>(height)};

    constexpr auto rt_stages = vk::ShaderStageFlagBits::eRaygenKHR |
                               vk::ShaderStageFlagBits::eMissKHR |
                               vk::ShaderStageFlagBits::eClosestHitKHR;

    cmd.pushConstants(m_pipeline->handle_layout(), rt_stages, 0,
                      sizeof(SkyLightPushConstants), &constants);

    // Trace rays
    cmd.traceRaysKHR(m_sbt->raygen_region(), m_sbt->miss_region(),
                     m_sbt->hit_region(),
                     vk::StridedDeviceAddressRegionKHR(), // callable (unused)
                     static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                     1);

    // Increment frame count AFTER tracing (so first frame is 0)
    m_frame_count++;

    return output.view;
}

} // namespace vw
