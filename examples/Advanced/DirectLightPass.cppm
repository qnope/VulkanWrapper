export module advanced:direct_light_pass;
import app;

export enum class DirectLightPassSlot {
    Albedo,
    Normal,
    Tangent,
    Bitangent,
    Position,
    DirectLight,
    IndirectRay
};

/**
 * @brief Output structure for the DirectLightPass G-Buffer
 */
export struct DirectLightPassOutput {
    std::shared_ptr<const vw::ImageView> albedo;
    std::shared_ptr<const vw::ImageView> normal;
    std::shared_ptr<const vw::ImageView> tangent;
    std::shared_ptr<const vw::ImageView> bitangent;
    std::shared_ptr<const vw::ImageView> position;
    std::shared_ptr<const vw::ImageView> direct_light;
    std::shared_ptr<const vw::ImageView> indirect_ray;
};

/**
 * @brief G-Buffer fill pass with per-fragment direct sun lighting
 *
 * This pass lazily allocates its G-Buffer images on first execute() call.
 * It computes direct sun lighting per-fragment using ray queries for shadows,
 * producing a DirectLight attachment as part of the G-Buffer.
 */
export class DirectLightPass : public vw::Subpass<DirectLightPassSlot> {
  public:
    static constexpr std::array<vk::Format, 7> default_color_formats = {
        vk::Format::eR8G8B8A8Unorm,      // albedo
        vk::Format::eR32G32B32A32Sfloat, // normal
        vk::Format::eR32G32B32A32Sfloat, // tangent
        vk::Format::eR32G32B32A32Sfloat, // bitangent
        vk::Format::eR32G32B32A32Sfloat, // position
        vk::Format::eR16G16B16A16Sfloat, // direct_light
        vk::Format::eR32G32B32A32Sfloat  // indirect_ray
    };

    DirectLightPass(
        std::shared_ptr<vw::Device> device,
        std::shared_ptr<vw::Allocator> allocator,
        vw::Model::Material::BindlessMaterialManager &material_manager,
        vk::AccelerationStructureKHR tlas,
        std::filesystem::path gbuffer_shader_dir,
        std::filesystem::path shader_include_dir,
        vk::Format depth_format = vk::Format::eD32Sfloat,
        std::span<const vk::Format> color_formats = default_color_formats)
        : Subpass(std::move(device), std::move(allocator))
        , m_color_formats(color_formats.begin(), color_formats.end())
        , m_material_manager(material_manager)
        , m_gbuffer_shader_dir(std::move(gbuffer_shader_dir))
        , m_shader_include_dir(std::move(shader_include_dir))
        , m_tlas(tlas)
        , m_sky_params_buffer(
              vw::create_buffer<vw::SkyParametersGPU, true,
                                vw::UniformBufferUsage>(*m_allocator, 1))
        , m_samples_buffer(vw::create_hemisphere_samples_buffer(*m_allocator))
        , m_noise_texture(std::make_unique<vw::NoiseTexture>(
              m_device, m_allocator, m_device->graphicsQueue()))
        , m_descriptor_pool(create_descriptor_pool(depth_format)) {}

    /**
     * @brief Execute the G-Buffer fill pass with direct sun lighting
     */
    template <typename UBOType>
    DirectLightPassOutput execute(
        vk::CommandBuffer cmd, vw::Barrier::ResourceTracker &tracker,
        vw::Width width, vw::Height height, size_t frame_index,
        const vw::Model::Scene &scene,
        std::shared_ptr<const vw::ImageView> depth_view,
        const vw::Buffer<UBOType, true, vw::UniformBufferUsage> &uniform_buffer,
        uint32_t frame_count, const vw::SkyParameters &sky_params,
        glm::vec3 camera_pos) {

        constexpr auto usageFlags = vk::ImageUsageFlagBits::eColorAttachment |
                                    vk::ImageUsageFlagBits::eInputAttachment |
                                    vk::ImageUsageFlagBits::eSampled |
                                    vk::ImageUsageFlagBits::eTransferSrc;

        // Lazy allocation of 7 G-Buffer images
        const auto &albedo =
            get_or_create_image(DirectLightPassSlot::Albedo, width, height,
                                frame_index, m_color_formats[0], usageFlags);
        const auto &normal =
            get_or_create_image(DirectLightPassSlot::Normal, width, height,
                                frame_index, m_color_formats[1], usageFlags);
        const auto &tangent =
            get_or_create_image(DirectLightPassSlot::Tangent, width, height,
                                frame_index, m_color_formats[2], usageFlags);
        const auto &bitangent =
            get_or_create_image(DirectLightPassSlot::Bitangent, width, height,
                                frame_index, m_color_formats[3], usageFlags);
        const auto &position =
            get_or_create_image(DirectLightPassSlot::Position, width, height,
                                frame_index, m_color_formats[4], usageFlags);
        const auto &direct_light =
            get_or_create_image(DirectLightPassSlot::DirectLight, width, height,
                                frame_index, m_color_formats[5], usageFlags);
        const auto &indirect_ray =
            get_or_create_image(DirectLightPassSlot::IndirectRay, width, height,
                                frame_index, m_color_formats[6], usageFlags);

        vk::Extent2D extent{static_cast<uint32_t>(width),
                            static_cast<uint32_t>(height)};

        // Write sky parameters to UBO
        auto sky_gpu = sky_params.to_gpu();
        m_sky_params_buffer.write(std::span(&sky_gpu, 1), 0);

        // Create descriptor set with uniform buffer + random sampling +
        // sky UBO + TLAS
        vw::DescriptorAllocator descriptor_allocator;
        descriptor_allocator.add_uniform_buffer(
            0, uniform_buffer.handle(), 0, uniform_buffer.size_bytes(),
            vk::PipelineStageFlagBits2::eVertexShader |
                vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eUniformRead);

        // binding 1: random samples SSBO
        descriptor_allocator.add_storage_buffer(
            1, m_samples_buffer.handle(), 0, m_samples_buffer.size_bytes(),
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
            4, m_tlas, vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eAccelerationStructureReadKHR);

        auto descriptor_set =
            m_descriptor_pool.allocate_set(descriptor_allocator);

        // Request resource states for barriers
        for (const auto &resource : descriptor_set.resources()) {
            tracker.request(resource);
        }

        // Request states for all output images
        std::array<const vw::CachedImage *, 7> cached_images = {
            &albedo,   &normal,       &tangent,     &bitangent,
            &position, &direct_light, &indirect_ray};

        for (const auto *cached : cached_images) {
            tracker.request(vw::Barrier::ImageState{
                .image = cached->image->handle(),
                .subresourceRange = cached->view->subresource_range(),
                .layout = vk::ImageLayout::eColorAttachmentOptimal,
                .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .access = vk::AccessFlagBits2::eColorAttachmentWrite});
        }

        // Depth image for reading (from Z-Pass)
        tracker.request(vw::Barrier::ImageState{
            .image = depth_view->image()->handle(),
            .subresourceRange = depth_view->subresource_range(),
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                     vk::PipelineStageFlagBits2::eLateFragmentTests,
            .access = vk::AccessFlagBits2::eDepthStencilAttachmentRead});

        // Request material resources (storage buffers + textures)
        for (const auto &resource : m_material_manager.get_resources()) {
            tracker.request(resource);
        }

        // Flush barriers
        tracker.flush(cmd);

        // Setup rendering attachments
        std::vector<vk::RenderingAttachmentInfo> color_attachments;
        for (size_t i = 0; i < cached_images.size(); ++i) {
            const auto *cached = cached_images[i];
            // DirectLight and IndirectRay clear to (0,0,0,0)
            auto clear_value =
                (i >= 5) ? vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f)
                         : vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
            color_attachments.push_back(
                vk::RenderingAttachmentInfo()
                    .setImageView(cached->view->handle())
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setClearValue(clear_value));
        }

        vk::RenderingAttachmentInfo depth_attachment =
            vk::RenderingAttachmentInfo()
                .setImageView(depth_view->handle())
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
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
        vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width),
                              static_cast<float>(extent.height), 0.0f, 1.0f);
        vk::Rect2D scissor({0, 0}, extent);
        cmd.setViewport(0, 1, &viewport);
        cmd.setScissor(0, 1, &scissor);

        // Bind uniform buffer descriptor set (set 0)
        auto uniform_descriptor_handle = descriptor_set.handle();

        // Find the texture descriptor set to bind for all materials
        std::optional<vk::DescriptorSet> texture_ds;
        for (const auto &[tag, handler] : m_material_manager.handlers()) {
            texture_ds = handler->additional_descriptor_set();
            if (texture_ds)
                break;
        }

        // Draw all mesh instances grouped by material type
        vw::Model::Material::MaterialTypeTag current_tag{0xFFFFFFFF};

        for (const auto &instance : scene.instances()) {
            auto material_type = instance.mesh.material_type_tag();

            // Bind pipeline and descriptor sets when material type changes
            if (material_type != current_tag) {
                current_tag = material_type;
                auto pipeline = m_mesh_renderer->pipeline_for(material_type);
                if (!pipeline) {
                    continue;
                }
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                 pipeline->handle());
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                       pipeline->layout().handle(), 0,
                                       uniform_descriptor_handle, nullptr);

                // Always bind textures/sampler descriptor set (set 1)
                if (texture_ds) {
                    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                           pipeline->layout().handle(), 1,
                                           *texture_ds, nullptr);
                }

                // Push frame_count and camera_pos for fragment shader
                struct {
                    uint32_t frame_count;
                    glm::vec3 camera_pos;
                } extra_push{frame_count, camera_pos};
                cmd.pushConstants(pipeline->layout().handle(),
                                  vk::ShaderStageFlagBits::eVertex |
                                      vk::ShaderStageFlagBits::eFragment,
                                  sizeof(vw::Model::MeshPushConstants),
                                  sizeof(extra_push), &extra_push);
            }

            auto pipeline = m_mesh_renderer->pipeline_for(material_type);
            if (pipeline) {
                instance.mesh.draw(cmd, pipeline->layout(), instance.transform);
            }
        }

        cmd.endRendering();

        return DirectLightPassOutput{.albedo = albedo.view,
                                     .normal = normal.view,
                                     .tangent = tangent.view,
                                     .bitangent = bitangent.view,
                                     .position = position.view,
                                     .direct_light = direct_light.view,
                                     .indirect_ray = indirect_ray.view};
    }

  private:
    vw::DescriptorPool create_descriptor_pool(vk::Format depth_format) {
        // Create descriptor layout for uniform buffer + random sampling +
        // sky UBO + TLAS
        m_descriptor_layout =
            vw::DescriptorSetLayoutBuilder(m_device)
                .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex |
                                         vk::ShaderStageFlagBits::eFragment,
                                     1) // binding 0: MVP UBO
                .with_storage_buffer(vk::ShaderStageFlagBits::eFragment,
                                     1) // binding 1: random samples
                .with_combined_image(vk::ShaderStageFlagBits::eFragment,
                                     1) // binding 2: noise texture
                .with_uniform_buffer(vk::ShaderStageFlagBits::eFragment,
                                     1) // binding 3: sky params UBO
                .with_acceleration_structure(
                    vk::ShaderStageFlagBits::eFragment) // binding 4: TLAS
                .build();

        // Create mesh renderer with pipelines
        m_mesh_renderer = create_renderer(depth_format);

        return vw::DescriptorPoolBuilder(m_device, m_descriptor_layout).build();
    }

    std::shared_ptr<vw::MeshRenderer> create_renderer(vk::Format depth_format) {
        vw::ShaderCompiler compiler;
        compiler.set_target_vulkan_version(vw_VK_API_VERSION_1_2);
        compiler.add_include_path(m_shader_include_dir);

        auto vertex_shader = compiler.compile_file_to_module(
            m_device, m_gbuffer_shader_dir / "gbuffer.vert");

        auto push_constant_range =
            vk::PushConstantRange()
                .setStageFlags(vk::ShaderStageFlagBits::eVertex |
                               vk::ShaderStageFlagBits::eFragment)
                .setOffset(0)
                .setSize(sizeof(vw::Model::MeshPushConstants) +
                         sizeof(uint32_t) + sizeof(glm::vec3));

        auto renderer = std::make_shared<vw::MeshRenderer>();

        // Find the shared texture descriptor set layout
        std::shared_ptr<const vw::DescriptorSetLayout> texture_layout;
        for (auto [tag, handler] : m_material_manager.handlers()) {
            texture_layout = handler->additional_descriptor_set_layout();
            if (texture_layout)
                break;
        }

        compiler.add_include_path(m_gbuffer_shader_dir);

        for (auto [tag, handler] : m_material_manager.handlers()) {
            auto source = "#version 460\n"
                          "#extension GL_GOOGLE_include_directive"
                          " : require\n"
                          "#include \"gbuffer_base.glsl\"\n"
                          "#include \"" +
                          handler->brdf_path().string() + "\"\n";

            auto fragment = compiler.compile_to_module(
                m_device, source, vk::ShaderStageFlagBits::eFragment,
                "gbuffer_dynamic.frag");

            auto layout_builder =
                vw::PipelineLayoutBuilder(m_device)
                    .with_descriptor_set_layout(m_descriptor_layout)
                    .with_push_constant_range(push_constant_range);

            if (texture_layout) {
                layout_builder.with_descriptor_set_layout(texture_layout);
            }

            vw::GraphicsPipelineBuilder builder(m_device,
                                                layout_builder.build());
            builder.add_vertex_binding<vw::FullVertex3D>()
                .add_shader(vk::ShaderStageFlagBits::eVertex, vertex_shader)
                .add_shader(vk::ShaderStageFlagBits::eFragment, fragment)
                .with_dynamic_viewport_scissor()
                .with_depth_test(false, vk::CompareOp::eEqual)
                .set_depth_format(depth_format);

            for (auto format : m_color_formats) {
                builder.add_color_attachment(format);
            }

            renderer->add_pipeline(tag, builder.build());
        }

        return renderer;
    }

    std::vector<vk::Format> m_color_formats;
    vw::Model::Material::BindlessMaterialManager &m_material_manager;
    std::filesystem::path m_gbuffer_shader_dir;
    std::filesystem::path m_shader_include_dir;
    vk::AccelerationStructureKHR m_tlas;

    // Sky parameters UBO
    vw::Buffer<vw::SkyParametersGPU, true, vw::UniformBufferUsage>
        m_sky_params_buffer;

    // Random sampling resources
    vw::DualRandomSampleBuffer m_samples_buffer;
    std::unique_ptr<vw::NoiseTexture> m_noise_texture;

    // Resources (order matters!)
    std::shared_ptr<vw::DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<vw::MeshRenderer> m_mesh_renderer;
    vw::DescriptorPool m_descriptor_pool;
};
