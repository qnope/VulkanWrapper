#include "VulkanWrapper/RenderPass/DirectLightPass.h"

#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/Vertex.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Model/Material/BindlessMaterialManager.h"
#include "VulkanWrapper/Model/Material/IMaterialTypeHandler.h"
#include "VulkanWrapper/Model/Mesh.h"
#include "VulkanWrapper/Model/Scene.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/RayTracing/RayTracedScene.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"

#include <cassert>
#include <glm/glm.hpp>

namespace vw {

DirectLightPass::DirectLightPass(
    std::shared_ptr<Device> device,
    std::shared_ptr<Allocator> allocator,
    const std::filesystem::path &shader_dir,
    const rt::RayTracedScene &ray_traced_scene,
    Model::Material::BindlessMaterialManager &material_manager,
    Formats formats)
    : RenderPass(std::move(device), std::move(allocator))
    , m_formats(formats)
    , m_ray_traced_scene(&ray_traced_scene)
    , m_material_manager(&material_manager)
    , m_descriptor_layout(
          DescriptorSetLayoutBuilder(m_device)
              .with_uniform_buffer(
                  vk::ShaderStageFlagBits::eVertex |
                      vk::ShaderStageFlagBits::eFragment,
                  1) // binding 0: MVP UBO
              .with_storage_buffer(
                  vk::ShaderStageFlagBits::eFragment,
                  1) // binding 1: random samples
              .with_combined_image(
                  vk::ShaderStageFlagBits::eFragment,
                  1) // binding 2: noise texture
              .with_uniform_buffer(
                  vk::ShaderStageFlagBits::eFragment,
                  1) // binding 3: sky params UBO
              .with_acceleration_structure(
                  vk::ShaderStageFlagBits::eFragment) // binding 4
              .build())
    , m_descriptor_pool(m_device, nullptr)
    , m_sky_params_buffer(
          create_buffer<SkyParametersGPU, true,
                        UniformBufferUsage>(*m_allocator, 1))
    , m_hemisphere_samples(
          create_hemisphere_samples_buffer(*m_allocator))
    , m_noise_texture(std::make_unique<NoiseTexture>(
          m_device, m_allocator,
          m_device->graphicsQueue())) {

    // Compile shaders and create pipelines per material type
    ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_2);
    compiler.add_include_path(shader_dir / "include");

    auto gbuffer_dir = shader_dir / "GBuffer";
    compiler.add_include_path(gbuffer_dir);

    auto vertex_shader = compiler.compile_file_to_module(
        m_device, gbuffer_dir / "gbuffer.vert");

    auto push_constant_range =
        vk::PushConstantRange()
            .setStageFlags(vk::ShaderStageFlagBits::eVertex |
                           vk::ShaderStageFlagBits::eFragment)
            .setOffset(0)
            .setSize(sizeof(Model::MeshPushConstants) +
                     sizeof(uint32_t) + sizeof(glm::vec3));

    // Find the shared texture descriptor set layout
    std::shared_ptr<const DescriptorSetLayout> texture_layout;
    for (auto [tag, handler] :
         m_material_manager->handlers()) {
        texture_layout =
            handler->additional_descriptor_set_layout();
        if (texture_layout)
            break;
    }

    std::array color_formats = {
        m_formats.albedo,       m_formats.normal,
        m_formats.tangent,      m_formats.bitangent,
        m_formats.position,     m_formats.direct_light,
        m_formats.indirect_ray};

    for (auto [tag, handler] :
         m_material_manager->handlers()) {
        auto source =
            "#version 460\n"
            "#extension GL_GOOGLE_include_directive"
            " : require\n"
            "#include \"gbuffer_base.glsl\"\n"
            "#include \"" +
            handler->brdf_path().string() + "\"\n";

        auto fragment = compiler.compile_to_module(
            m_device, source,
            vk::ShaderStageFlagBits::eFragment,
            "gbuffer_dynamic.frag");

        auto layout_builder =
            PipelineLayoutBuilder(m_device)
                .with_descriptor_set_layout(
                    m_descriptor_layout)
                .with_push_constant_range(
                    push_constant_range);

        if (texture_layout) {
            layout_builder.with_descriptor_set_layout(
                texture_layout);
        }

        GraphicsPipelineBuilder builder(
            m_device, layout_builder.build());
        builder.add_vertex_binding<FullVertex3D>()
            .add_shader(vk::ShaderStageFlagBits::eVertex,
                        vertex_shader)
            .add_shader(vk::ShaderStageFlagBits::eFragment,
                        fragment)
            .with_dynamic_viewport_scissor()
            .with_depth_test(false, vk::CompareOp::eEqual)
            .set_depth_format(m_formats.depth);

        for (auto format : color_formats) {
            builder.add_color_attachment(format);
        }

        m_mesh_renderer.add_pipeline(tag, builder.build());
    }

    // Create descriptor pool
    m_descriptor_pool =
        DescriptorPoolBuilder(m_device, m_descriptor_layout)
            .build();
}

std::vector<Slot> DirectLightPass::input_slots() const {
    return {Slot::Depth};
}

std::vector<Slot> DirectLightPass::output_slots() const {
    return {Slot::Albedo,      Slot::Normal,
            Slot::Tangent,     Slot::Bitangent,
            Slot::Position,    Slot::DirectLight,
            Slot::IndirectRay};
}

void DirectLightPass::set_uniform_buffer(
    const BufferBase &ubo) {
    m_uniform_buffer = &ubo;
}

void DirectLightPass::set_sky_parameters(
    const SkyParameters &params) {
    m_sky_params = params;
}

void DirectLightPass::set_camera_position(
    const glm::vec3 &pos) {
    m_camera_pos = pos;
}

void DirectLightPass::set_frame_count(uint32_t count) {
    m_frame_count = count;
}

void DirectLightPass::execute(
    vk::CommandBuffer cmd,
    Barrier::ResourceTracker &tracker,
    Width width, Height height,
    size_t frame_index) {

    assert(m_uniform_buffer &&
           "DirectLightPass: uniform buffer not set");
    assert(m_ray_traced_scene &&
           "DirectLightPass: ray traced scene not set");

    // Get depth view from ZPass input
    auto depth_view = get_input(Slot::Depth).view;

    constexpr auto usage_flags =
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eInputAttachment |
        vk::ImageUsageFlagBits::eSampled |
        vk::ImageUsageFlagBits::eTransferSrc;

    // Lazy allocation of 7 G-Buffer images
    const auto &albedo = get_or_create_image(
        Slot::Albedo, width, height, frame_index,
        m_formats.albedo, usage_flags);
    const auto &normal = get_or_create_image(
        Slot::Normal, width, height, frame_index,
        m_formats.normal, usage_flags);
    const auto &tangent = get_or_create_image(
        Slot::Tangent, width, height, frame_index,
        m_formats.tangent, usage_flags);
    const auto &bitangent = get_or_create_image(
        Slot::Bitangent, width, height, frame_index,
        m_formats.bitangent, usage_flags);
    const auto &position = get_or_create_image(
        Slot::Position, width, height, frame_index,
        m_formats.position, usage_flags);
    const auto &direct_light = get_or_create_image(
        Slot::DirectLight, width, height, frame_index,
        m_formats.direct_light, usage_flags);
    const auto &indirect_ray = get_or_create_image(
        Slot::IndirectRay, width, height, frame_index,
        m_formats.indirect_ray, usage_flags);

    vk::Extent2D extent{static_cast<uint32_t>(width),
                        static_cast<uint32_t>(height)};

    // Write sky parameters to UBO
    auto sky_gpu = m_sky_params.to_gpu();
    m_sky_params_buffer.write(std::span(&sky_gpu, 1), 0);

    // Create descriptor set
    DescriptorAllocator descriptor_allocator;
    descriptor_allocator.add_uniform_buffer(
        0, m_uniform_buffer->handle(), 0,
        m_uniform_buffer->size_bytes(),
        vk::PipelineStageFlagBits2::eVertexShader |
            vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eUniformRead);

    // binding 1: random samples SSBO
    descriptor_allocator.add_storage_buffer(
        1, m_hemisphere_samples.handle(), 0,
        m_hemisphere_samples.size_bytes(),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);

    // binding 2: noise texture
    descriptor_allocator.add_combined_image(
        2, m_noise_texture->combined_image(),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);

    // binding 3: sky parameters UBO
    descriptor_allocator.add_uniform_buffer(
        3, m_sky_params_buffer.handle(), 0,
        m_sky_params_buffer.size_bytes(),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eUniformRead);

    // binding 4: TLAS for shadow ray queries
    descriptor_allocator.add_acceleration_structure(
        4, m_ray_traced_scene->tlas_handle(),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eAccelerationStructureReadKHR);

    auto descriptor_set =
        m_descriptor_pool.allocate_set(descriptor_allocator);

    // Request resource states for barriers
    for (const auto &resource :
         descriptor_set.resources()) {
        tracker.request(resource);
    }

    // Request states for all output images
    std::array<const CachedImage *, 7> cached_images = {
        &albedo,   &normal,       &tangent,
        &bitangent, &position,    &direct_light,
        &indirect_ray};

    for (const auto *cached : cached_images) {
        tracker.request(Barrier::ImageState{
            .image = cached->image->handle(),
            .subresourceRange =
                cached->view->subresource_range(),
            .layout =
                vk::ImageLayout::eColorAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::
                eColorAttachmentOutput,
            .access = vk::AccessFlagBits2::
                eColorAttachmentWrite});
    }

    // Depth image for reading (from Z-Pass)
    tracker.request(Barrier::ImageState{
        .image = depth_view->image()->handle(),
        .subresourceRange =
            depth_view->subresource_range(),
        .layout =
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .stage =
            vk::PipelineStageFlagBits2::eEarlyFragmentTests |
            vk::PipelineStageFlagBits2::eLateFragmentTests,
        .access = vk::AccessFlagBits2::
            eDepthStencilAttachmentRead});

    // Request material resources (storage buffers + textures)
    for (const auto &resource :
         m_material_manager->get_resources()) {
        tracker.request(resource);
    }

    // Flush barriers
    tracker.flush(cmd);

    // Setup rendering attachments
    std::vector<vk::RenderingAttachmentInfo>
        color_attachments;
    for (size_t i = 0; i < cached_images.size(); ++i) {
        const auto *cached = cached_images[i];
        // DirectLight and IndirectRay clear to (0,0,0,0)
        auto clear_value =
            (i >= 5)
                ? vk::ClearColorValue(
                      0.0f, 0.0f, 0.0f, 0.0f)
                : vk::ClearColorValue(
                      0.0f, 0.0f, 0.0f, 1.0f);
        color_attachments.push_back(
            vk::RenderingAttachmentInfo()
                .setImageView(cached->view->handle())
                .setImageLayout(
                    vk::ImageLayout::
                        eColorAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setClearValue(clear_value));
    }

    vk::RenderingAttachmentInfo depth_attachment =
        vk::RenderingAttachmentInfo()
            .setImageView(depth_view->handle())
            .setImageLayout(
                vk::ImageLayout::
                    eDepthStencilAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eLoad)
            .setStoreOp(vk::AttachmentStoreOp::eStore);

    vk::RenderingInfo rendering_info =
        vk::RenderingInfo()
            .setRenderArea(vk::Rect2D({0, 0}, extent))
            .setLayerCount(1)
            .setColorAttachments(color_attachments)
            .setPDepthAttachment(&depth_attachment);

    cmd.beginRendering(rendering_info);

    // Set viewport and scissor
    vk::Viewport viewport(
        0.0f, 0.0f,
        static_cast<float>(extent.width),
        static_cast<float>(extent.height), 0.0f, 1.0f);
    vk::Rect2D scissor({0, 0}, extent);
    cmd.setViewport(0, 1, &viewport);
    cmd.setScissor(0, 1, &scissor);

    // Bind uniform buffer descriptor set (set 0)
    auto uniform_descriptor_handle =
        descriptor_set.handle();

    // Find the texture descriptor set to bind
    std::optional<vk::DescriptorSet> texture_ds;
    for (const auto &[tag, handler] :
         m_material_manager->handlers()) {
        texture_ds = handler->additional_descriptor_set();
        if (texture_ds)
            break;
    }

    // Draw all mesh instances grouped by material type
    Model::Material::MaterialTypeTag current_tag{
        0xFFFFFFFF};

    const auto &scene = m_ray_traced_scene->scene();
    for (const auto &instance : scene.instances()) {
        auto material_type =
            instance.mesh.material_type_tag();

        // Bind pipeline and descriptors when material
        // type changes
        if (material_type != current_tag) {
            current_tag = material_type;
            auto pipeline =
                m_mesh_renderer.pipeline_for(
                    material_type);
            if (!pipeline) {
                continue;
            }
            cmd.bindPipeline(
                vk::PipelineBindPoint::eGraphics,
                pipeline->handle());
            cmd.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                pipeline->layout().handle(), 0,
                uniform_descriptor_handle, nullptr);

            // Always bind texture descriptor set (set 1)
            if (texture_ds) {
                cmd.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    pipeline->layout().handle(), 1,
                    *texture_ds, nullptr);
            }

            // Push frame_count and camera_pos for
            // fragment shader
            struct {
                uint32_t frame_count;
                glm::vec3 camera_pos;
            } extra_push{m_frame_count, m_camera_pos};
            cmd.pushConstants(
                pipeline->layout().handle(),
                vk::ShaderStageFlagBits::eVertex |
                    vk::ShaderStageFlagBits::eFragment,
                sizeof(Model::MeshPushConstants),
                sizeof(extra_push), &extra_push);
        }

        auto pipeline =
            m_mesh_renderer.pipeline_for(material_type);
        if (pipeline) {
            instance.mesh.draw(
                cmd, pipeline->layout(),
                instance.transform);
        }
    }

    cmd.endRendering();
}

} // namespace vw
