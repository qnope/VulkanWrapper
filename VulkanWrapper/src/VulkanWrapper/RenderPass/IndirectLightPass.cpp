#include "VulkanWrapper/RenderPass/IndirectLightPass.h"

#include "VulkanWrapper/Pipeline/PipelineLayout.h"

namespace vw {

IndirectLightPass::IndirectLightPass(
    std::shared_ptr<Device> device,
    std::shared_ptr<Allocator> allocator,
    const std::filesystem::path &shader_dir,
    const rt::as::TopLevelAccelerationStructure &tlas,
    const rt::GeometryReferenceBuffer &geometry_buffer,
    Model::Material::BindlessMaterialManager &material_manager,
    vk::Format output_format)
    : RenderPass(device, allocator)
    , m_tlas(&tlas)
    , m_geometry_buffer(&geometry_buffer)
    , m_material_manager(&material_manager)
    , m_output_format(output_format)
    , m_sampler(SamplerBuilder(m_device).build())
    , m_descriptor_pool(DescriptorPool(m_device, nullptr))
    , m_texture_descriptor_pool(
          DescriptorPool(m_device, nullptr)) {
    // Create pipeline and SBT
    create_pipeline_and_sbt(shader_dir);
}

void IndirectLightPass::create_pipeline_and_sbt(
    const std::filesystem::path &shader_dir) {
    // Create descriptor layout for RT pipeline (set 0):
    // binding 0: accelerationStructureEXT (TLAS)
    // binding 1: sampler2D (G-Buffer position)
    // binding 2: sampler2D (G-Buffer normal)
    // binding 3: image2D storage (output - read/write)
    // binding 4: sampler2D (G-Buffer albedo)
    // binding 5: sampler2D (Ambient Occlusion)
    // binding 6: sampler2D (G-Buffer indirect_ray)
    // binding 7: SSBO (geometry references)
    constexpr auto rt_stages =
        vk::ShaderStageFlagBits::eRaygenKHR |
        vk::ShaderStageFlagBits::eMissKHR |
        vk::ShaderStageFlagBits::eClosestHitKHR;

    m_descriptor_layout =
        DescriptorSetLayoutBuilder(m_device)
            .with_acceleration_structure(rt_stages)
            .with_combined_image(rt_stages, 1)
            .with_combined_image(rt_stages, 1)
            .with_storage_image(rt_stages, 1)
            .with_combined_image(rt_stages, 1)
            .with_combined_image(rt_stages, 1)
            .with_combined_image(rt_stages, 1)
            .with_storage_buffer(rt_stages, 1)
            .build();

    // Create texture descriptor layout (set 1)
    m_texture_descriptor_layout =
        DescriptorSetLayoutBuilder(m_device)
            .with_sampler(
                vk::ShaderStageFlagBits::eClosestHitKHR)
            .with_sampled_images_bindless(
                vk::ShaderStageFlagBits::eClosestHitKHR,
                Model::Material::BindlessTextureManager::
                    MAX_TEXTURES)
            .build();

    // Create pipeline layout with both descriptor set layouts
    auto pipeline_layout =
        PipelineLayoutBuilder(m_device)
            .with_descriptor_set_layout(m_descriptor_layout)
            .with_descriptor_set_layout(
                m_texture_descriptor_layout)
            .with_push_constant_range(vk::PushConstantRange(
                rt_stages, 0,
                sizeof(IndirectLightPushConstants)))
            .build();

    // Compile shaders
    ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_2);
    compiler.add_include_path(shader_dir / "include");

    auto raygen_shader = compiler.compile_file_to_module(
        m_device, shader_dir / "indirect_light.rgen");
    auto miss_shader = compiler.compile_file_to_module(
        m_device, shader_dir / "indirect_light.rmiss");

    // Build RT pipeline with per-material closest hit shaders
    auto builder = rt::RayTracingPipelineBuilder(
        m_device, m_allocator, std::move(pipeline_layout));
    builder.set_ray_generation_shader(raygen_shader)
        .add_miss_shader(miss_shader);

    for (auto [tag, handler] :
         m_material_manager->ordered_handlers()) {
        auto source =
            "#version 460\n"
            "#extension GL_GOOGLE_include_directive"
            " : require\n"
            "#include \"indirect_light_base.glsl\"\n"
            "#include \"" +
            handler->brdf_path().string() + "\"\n";

        auto hit_module = compiler.compile_to_module(
            m_device, source,
            vk::ShaderStageFlagBits::eClosestHitKHR,
            "indirect_light_dynamic.rchit");
        builder.add_closest_hit_shader(hit_module);
    }

    m_pipeline = std::make_unique<rt::RayTracingPipeline>(
        builder.build());

    // Create shader binding table
    m_sbt = std::make_unique<rt::ShaderBindingTable>(
        m_allocator, m_pipeline->ray_generation_handle());

    // Add miss shader record
    auto miss_handles = m_pipeline->miss_handles();
    if (!miss_handles.empty()) {
        m_sbt->add_miss_record(miss_handles[0]);
    }

    // Add all hit shader records (one per material type)
    auto hit_handles = m_pipeline->closest_hit_handles();
    for (const auto &handle : hit_handles) {
        m_sbt->add_hit_record(handle);
    }

    // Create descriptor pools
    m_descriptor_pool =
        DescriptorPoolBuilder(m_device, m_descriptor_layout)
            .build();
    m_texture_descriptor_pool =
        DescriptorPoolBuilder(m_device,
                              m_texture_descriptor_layout)
            .with_update_after_bind()
            .build();
}

void IndirectLightPass::execute(
    vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
    Width width, Height height, size_t /*frame_index*/) {

    // Get input views from wired slots
    auto position_view = get_input(Slot::Position).view;
    auto normal_view = get_input(Slot::Normal).view;
    auto albedo_view = get_input(Slot::Albedo).view;
    auto ao_view = get_input(Slot::AmbientOcclusion).view;
    auto indirect_ray_view = get_input(Slot::IndirectRay).view;

    // Use fixed frame_index=0 so the image is shared across all
    // swapchain frames. This is required for progressive
    // accumulation to work correctly.
    constexpr size_t indirect_light_frame_index = 0;

    // Single accumulation buffer (storage image for RT)
    const auto &output = get_or_create_image(
        Slot::IndirectLight, width, height,
        indirect_light_frame_index, m_output_format,
        vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc);

    // Create descriptor set with current input images
    DescriptorAllocator descriptor_allocator;

    // binding 0: TLAS
    descriptor_allocator.add_acceleration_structure(
        0, m_tlas->handle(),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
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

    // binding 3: Output storage image (read/write)
    descriptor_allocator.add_storage_image(
        3, *output.view,
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eShaderRead |
            vk::AccessFlagBits2::eShaderWrite);

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

    // binding 6: Indirect ray direction G-Buffer
    descriptor_allocator.add_combined_image(
        6, CombinedImage(indirect_ray_view, m_sampler),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eShaderRead);

    // binding 7: Geometry reference buffer
    descriptor_allocator.add_storage_buffer(
        7, m_geometry_buffer->handle(), 0,
        m_geometry_buffer->size_bytes(),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eShaderRead);

    auto descriptor_set =
        m_descriptor_pool.allocate_set(descriptor_allocator);

    // Allocate texture descriptor set for per-material hit
    // shaders
    auto tex_desc_set =
        m_texture_descriptor_pool.allocate_set();
    {
        DescriptorAllocator tex_alloc;
        tex_alloc.add_sampler(
            0,
            m_material_manager->texture_manager().sampler());
        m_texture_descriptor_pool.update_set(
            tex_desc_set.handle(), tex_alloc);
    }
    m_material_manager->texture_manager()
        .write_image_descriptors(tex_desc_set.handle(), 1);

    // Request resource states for barriers
    for (const auto &resource : descriptor_set.resources()) {
        tracker.request(resource);
    }

    // Track texture resources for per-material hit shaders
    for (const auto &resource :
         m_material_manager->texture_manager()
             .get_resources()) {
        auto image_state =
            std::get<Barrier::ImageState>(resource);
        image_state.stage =
            vk::PipelineStageFlagBits2::eRayTracingShaderKHR;
        tracker.request(image_state);
    }

    // Output image needs to be in General layout for storage
    // image access
    tracker.request(Barrier::ImageState{
        .image = output.image->handle(),
        .subresourceRange =
            output.view->subresource_range(),
        .layout = vk::ImageLayout::eGeneral,
        .stage =
            vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .access = vk::AccessFlagBits2::eShaderRead |
                  vk::AccessFlagBits2::eShaderWrite});

    // Flush barriers
    tracker.flush(cmd);

    // Bind RT pipeline
    cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,
                     m_pipeline->handle());

    // Bind both descriptor sets
    auto descriptor_handle = descriptor_set.handle();
    auto tex_descriptor_handle = tex_desc_set.handle();
    std::array desc_sets = {descriptor_handle,
                            tex_descriptor_handle};
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eRayTracingKHR,
        m_pipeline->handle_layout(), 0, desc_sets, {});

    // Push constants
    IndirectLightPushConstants constants{
        .sky = m_sky_params.to_gpu(),
        .frame_count = m_frame_count,
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height)};

    constexpr auto rt_stages =
        vk::ShaderStageFlagBits::eRaygenKHR |
        vk::ShaderStageFlagBits::eMissKHR |
        vk::ShaderStageFlagBits::eClosestHitKHR;

    cmd.pushConstants(m_pipeline->handle_layout(), rt_stages,
                      0, sizeof(IndirectLightPushConstants),
                      &constants);

    // Trace rays
    cmd.traceRaysKHR(
        m_sbt->raygen_region(), m_sbt->miss_region(),
        m_sbt->hit_region(),
        vk::StridedDeviceAddressRegionKHR(), // callable
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height), 1);

    // Increment frame count AFTER tracing (first frame is 0)
    m_frame_count++;
}

} // namespace vw
