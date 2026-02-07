#pragma once

#include "AmbientOcclusionPass.h"
#include "ColorPass.h"
#include "ZPass.h"
#include <filesystem>
#include <memory>
#include <VulkanWrapper/Memory/Buffer.h>
#include <VulkanWrapper/Model/Material/BindlessMaterialManager.h>
#include <VulkanWrapper/Model/Scene.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>
#include <VulkanWrapper/RenderPass/IndirectLightPass.h>
#include <VulkanWrapper/RenderPass/SkyParameters.h>
#include <VulkanWrapper/RenderPass/SkyPass.h>
#include <VulkanWrapper/RenderPass/SunLightPass.h>
#include <VulkanWrapper/RenderPass/ToneMappingPass.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>
#include <VulkanWrapper/Vulkan/Device.h>

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
class DeferredRenderingManager {
  public:
    DeferredRenderingManager(
        std::shared_ptr<vw::Device> device,
        std::shared_ptr<vw::Allocator> allocator,
        vw::Model::Material::BindlessMaterialManager &material_manager,
        const vw::rt::RayTracedScene &ray_traced_scene,
        const std::filesystem::path &shader_dir,
        vk::Format depth_format = vk::Format::eD32Sfloat,
        vk::Format ao_format = vk::Format::eR32G32B32A32Sfloat,
        vk::Format light_format = vk::Format::eR32G32B32A32Sfloat)
        : m_device(std::move(device))
        , m_allocator(std::move(allocator))
        , m_ray_traced_scene(ray_traced_scene) {

        // Create functional passes - each owns its output images lazily
        m_zpass = std::make_unique<ZPass>(m_device, m_allocator, depth_format);

        m_color_pass = std::make_unique<ColorPass>(
            m_device, m_allocator, material_manager, depth_format);

        m_ao_pass = std::make_unique<AmbientOcclusionPass>(
            m_device, m_allocator, ray_traced_scene.tlas_handle(), ao_format);

        m_sky_pass = std::make_unique<vw::SkyPass>(
            m_device, m_allocator, shader_dir, light_format, depth_format);

        m_sun_light_pass = std::make_unique<vw::SunLightPass>(
            m_device, m_allocator, shader_dir, ray_traced_scene.tlas_handle(),
            light_format);

        m_indirect_light_pass = std::make_unique<vw::IndirectLightPass>(
            m_device, m_allocator, shader_dir, ray_traced_scene.tlas(),
            ray_traced_scene.geometry_buffer(), light_format);

        m_tone_mapping_pass = std::make_unique<vw::ToneMappingPass>(
            m_device, m_allocator, shader_dir);
    }

    /**
     * @brief Execute the full deferred rendering pipeline
     *
     * This method chains all rendering passes functionally:
     * 1. Z-Pass: depth prepass
     * 2. Color Pass: G-Buffer fill
     * 3. AO Pass: screen-space ambient occlusion
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

        // 1. Z-Pass: depth prepass
        auto depth_view = m_zpass->execute(cmd, tracker, width, height,
                                           frame_index, scene, uniform_buffer);

        // 2. Color Pass: G-Buffer fill
        auto gbuffer =
            m_color_pass->execute(cmd, tracker, width, height, frame_index,
                                  scene, depth_view, uniform_buffer);

        // 3. AO Pass: progressive ambient occlusion (1 sample per frame)
        auto ao_view = m_ao_pass->execute(
            cmd, tracker, width, height, depth_view, gbuffer.position,
            gbuffer.normal, gbuffer.tangent, gbuffer.bitangent, ao_radius);

        // 4. Sky Pass: render sky where depth == 1.0 (far plane)
        auto light_view = m_sky_pass->execute(
            cmd, tracker, width, height, frame_index, depth_view, sky_params,
            ubo_data.inverseViewProj);

        // 5. Sun Light Pass: add sun lighting with ray-traced shadows
        //    (direct light only - ambient/AO is handled by IndirectLightPass)
        m_sun_light_pass->execute(cmd, tracker,
                                  light_view,       // from SkyPass
                                  depth_view,       // from ZPass
                                  gbuffer.color,    // albedo from ColorPass
                                  gbuffer.position, // from ColorPass
                                  gbuffer.normal,   // from ColorPass
                                  sky_params);

        // 6. Indirect Light Pass: progressive indirect sky lighting with AO
        auto indirect_light_view = m_indirect_light_pass->execute(
            cmd, tracker, width, height, gbuffer.position, gbuffer.normal,
            gbuffer.color, ao_view, gbuffer.tangent, gbuffer.bitangent,
            sky_params);

        // 7. Tone Mapping Pass: combine direct + indirect and convert HDR to
        // LDR Note: Both direct and indirect light are in physical units
        // (cd/m²). We use luminance_scale = 10000 to normalize these values
        // before tone mapping.
        return m_tone_mapping_pass->execute(
            cmd, tracker, width, height, frame_index,
            light_view,          // direct light (sky + sun)
            indirect_light_view, // indirect light from indirect light pass
            1.0f, // indirect_intensity - physically correct addition
            vw::ToneMappingOperator::ACES,
            1.0f,      // exposure
            4.0f,      // white_point
            10000.0f); // luminance_scale - normalize physical cd/m² values
    }

    /** @brief Access the Z-Pass for custom usage */
    ZPass &zpass() { return *m_zpass; }
    const ZPass &zpass() const { return *m_zpass; }

    /** @brief Access the Color Pass for custom usage */
    ColorPass &color_pass() { return *m_color_pass; }
    const ColorPass &color_pass() const { return *m_color_pass; }

    /** @brief Access the AO Pass for custom usage */
    AmbientOcclusionPass &ao_pass() { return *m_ao_pass; }
    const AmbientOcclusionPass &ao_pass() const { return *m_ao_pass; }

    /** @brief Access the Sky Pass for custom usage */
    vw::SkyPass &sky_pass() { return *m_sky_pass; }
    const vw::SkyPass &sky_pass() const { return *m_sky_pass; }

    /** @brief Access the Sun Light Pass for custom usage */
    vw::SunLightPass &sun_light_pass() { return *m_sun_light_pass; }
    const vw::SunLightPass &sun_light_pass() const { return *m_sun_light_pass; }

    /** @brief Access the Indirect Light Pass for custom usage */
    vw::IndirectLightPass &indirect_light_pass() { return *m_indirect_light_pass; }
    const vw::IndirectLightPass &indirect_light_pass() const { return *m_indirect_light_pass; }

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
    std::unique_ptr<ColorPass> m_color_pass;
    std::unique_ptr<AmbientOcclusionPass> m_ao_pass;
    std::unique_ptr<vw::SkyPass> m_sky_pass;
    std::unique_ptr<vw::SunLightPass> m_sun_light_pass;
    std::unique_ptr<vw::IndirectLightPass> m_indirect_light_pass;
    std::unique_ptr<vw::ToneMappingPass> m_tone_mapping_pass;
};
