export module advanced:deferred_rendering_manager;
import :z_pass;
import :direct_light_pass;
import :ambient_occlusion_pass;
import app;

/**
 * @brief Manages deferred rendering passes with functional API and lazy
 * allocation
 *
 * This class orchestrates the deferred rendering pipeline using functional
 * passes. Each pass lazily allocates its output images on first use.
 *
 * @code
 * DeferredRenderingManager renderer(device, allocator, material_manager,
 *                                   ray_traced_scene);
 *
 * // In render loop:
 * auto ao_view = renderer.execute(cmd, tracker, width, height, frame_index,
 *                                 uniform_buffer);
 * transfer.blit(cmd, ao_view->image(), swapchain_image);
 * @endcode
 */
export class DeferredRenderingManager {
  public:
    DeferredRenderingManager(
        std::shared_ptr<vw::Device> device,
        std::shared_ptr<vw::Allocator> allocator,
        vw::Model::Material::BindlessMaterialManager &material_manager,
        const vw::rt::RayTracedScene &ray_traced_scene,
        const std::filesystem::path &shader_dir,
        const std::filesystem::path &example_shader_dir,
        vk::Format depth_format = vk::Format::eD32Sfloat,
        vk::Format ao_format = vk::Format::eR32G32B32A32Sfloat,
        vk::Format light_format = vk::Format::eR32G32B32A32Sfloat)
        : m_device(std::move(device))
        , m_allocator(std::move(allocator))
        , m_ray_traced_scene(ray_traced_scene) {

        auto gbuffer_dir = example_shader_dir / "GBuffer";
        auto include_dir = shader_dir / "include";

        // Create functional passes - each owns its output images lazily
        m_zpass = std::make_unique<ZPass>(m_device, m_allocator, gbuffer_dir,
                                          depth_format);

        m_direct_light_pass = std::make_unique<DirectLightPass>(
            m_device, m_allocator, material_manager,
            ray_traced_scene.tlas_handle(), std::move(gbuffer_dir), include_dir,
            depth_format);

        m_ao_pass = std::make_unique<AmbientOcclusionPass>(
            m_device, m_allocator, ray_traced_scene.tlas_handle(), shader_dir,
            example_shader_dir / "post-process", ao_format);

        m_sky_pass = std::make_unique<vw::SkyPass>(
            m_device, m_allocator, shader_dir, light_format, depth_format);

        m_indirect_light_pass = std::make_unique<vw::IndirectLightPass>(
            m_device, m_allocator, shader_dir, ray_traced_scene.tlas(),
            ray_traced_scene.geometry_buffer(), material_manager, light_format);

        m_tone_mapping_pass = std::make_unique<vw::ToneMappingPass>(
            m_device, m_allocator, shader_dir);
    }

    /**
     * @brief Execute the full deferred rendering pipeline
     *
     * This method chains all rendering passes functionally:
     * 1. Z-Pass: depth prepass
     * 2. DirectLight Pass: G-Buffer fill with per-fragment sun lighting
     * 3. AO Pass: screen-space ambient occlusion
     * 4. Sky Pass: sky radiance where depth == 1.0
     * 5. Indirect Light Pass: progressive indirect sky lighting
     * 6. Tone Mapping: combine sky + direct + indirect, HDR to LDR
     *
     * @param cmd Command buffer to record into
     * @param tracker Resource tracker for barrier management
     * @param width Render width
     * @param height Render height
     * @param frame_index Frame index for multi-buffering
     * @param uniform_buffer Uniform buffer with view/projection matrices
     * @param sky_params Sky and sun parameters
     * @param ao_radius AO sampling radius (default: 100.0)
     * @return The final light image view (can be blitted to swapchain)
     */
    template <typename UBOType>
    std::shared_ptr<const vw::ImageView> execute(
        vk::CommandBuffer cmd, vw::Barrier::ResourceTracker &tracker,
        vw::Width width, vw::Height height, size_t frame_index,
        const vw::Buffer<UBOType, true, vw::UniformBufferUsage> &uniform_buffer,
        const vw::SkyParameters &sky_params, float ao_radius = 100.0f) {

        const auto &scene = m_ray_traced_scene.scene();

        // Read UBO data for matrices
        auto ubo_data = uniform_buffer.read_as_vector(0, 1)[0];
        glm::vec3 camera_pos = glm::vec3(glm::inverse(ubo_data.view)[3]);

        // 1. Z-Pass: depth prepass
        auto depth_view = m_zpass->execute(cmd, tracker, width, height,
                                           frame_index, scene, uniform_buffer);

        // 2. DirectLight Pass: G-Buffer fill with per-fragment sun lighting
        auto gbuffer = m_direct_light_pass->execute(
            cmd, tracker, width, height, frame_index, scene, depth_view,
            uniform_buffer, m_indirect_light_pass->get_frame_count(),
            sky_params, camera_pos);

        // 3. AO Pass: progressive ambient occlusion (1 sample per frame)
        auto ao_view = m_ao_pass->execute(
            cmd, tracker, width, height, depth_view, gbuffer.position,
            gbuffer.normal, gbuffer.tangent, gbuffer.bitangent, ao_radius);

        // 4. Sky Pass: render sky where depth == 1.0 (far plane)
        auto sky_view = m_sky_pass->execute(cmd, tracker, width, height,
                                            frame_index, depth_view, sky_params,
                                            ubo_data.inverseViewProj);

        // 5. Indirect Light Pass: progressive indirect sky lighting with AO
        auto indirect_light_view = m_indirect_light_pass->execute(
            cmd, tracker, width, height, gbuffer.position, gbuffer.normal,
            gbuffer.albedo, ao_view, gbuffer.indirect_ray, sky_params);

        // 6. Tone Mapping Pass: combine sky + direct + indirect, HDR to LDR
        return m_tone_mapping_pass->execute(
            cmd, tracker, width, height, frame_index,
            sky_view,             // sky radiance (from SkyPass)
            gbuffer.direct_light, // direct sun light (from DirectLightPass)
            indirect_light_view,  // indirect light
            1.0f, // indirect_intensity - physically correct addition
            vw::ToneMappingOperator::ACES,
            1.0f,      // exposure
            4.0f,      // white_point
            10000.0f); // luminance_scale - normalize physical cd/m^2 values
    }

    /** @brief Access the Z-Pass for custom usage */
    ZPass &zpass() { return *m_zpass; }
    const ZPass &zpass() const { return *m_zpass; }

    /** @brief Access the DirectLight Pass for custom usage */
    DirectLightPass &direct_light_pass() { return *m_direct_light_pass; }
    const DirectLightPass &direct_light_pass() const {
        return *m_direct_light_pass;
    }

    /** @brief Access the AO Pass for custom usage */
    AmbientOcclusionPass &ao_pass() { return *m_ao_pass; }
    const AmbientOcclusionPass &ao_pass() const { return *m_ao_pass; }

    /** @brief Access the Sky Pass for custom usage */
    vw::SkyPass &sky_pass() { return *m_sky_pass; }
    const vw::SkyPass &sky_pass() const { return *m_sky_pass; }

    /** @brief Access the Indirect Light Pass for custom usage */
    vw::IndirectLightPass &indirect_light_pass() {
        return *m_indirect_light_pass;
    }
    const vw::IndirectLightPass &indirect_light_pass() const {
        return *m_indirect_light_pass;
    }

    /** @brief Access the Tone Mapping Pass for custom usage */
    vw::ToneMappingPass &tone_mapping_pass() { return *m_tone_mapping_pass; }
    const vw::ToneMappingPass &tone_mapping_pass() const {
        return *m_tone_mapping_pass;
    }

    /** @brief Reset progressive state (call on resize or camera move) */
    void reset() {
        m_ao_pass->reset_accumulation();
        m_indirect_light_pass->reset_accumulation();
    }

  private:
    std::shared_ptr<vw::Device> m_device;
    std::shared_ptr<vw::Allocator> m_allocator;
    const vw::rt::RayTracedScene &m_ray_traced_scene;

    // Functional passes (each owns its output images lazily)
    std::unique_ptr<ZPass> m_zpass;
    std::unique_ptr<DirectLightPass> m_direct_light_pass;
    std::unique_ptr<AmbientOcclusionPass> m_ao_pass;
    std::unique_ptr<vw::SkyPass> m_sky_pass;
    std::unique_ptr<vw::IndirectLightPass> m_indirect_light_pass;
    std::unique_ptr<vw::ToneMappingPass> m_tone_mapping_pass;
};
