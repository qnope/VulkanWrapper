#pragma once

#include "AmbientOcclusionPass.h"
#include "ColorPass.h"
#include "SkyPass.h"
#include "SunLightPass.h"
#include "ZPass.h"
#include <memory>
#include <VulkanWrapper/Memory/Buffer.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/Model/Scene.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>
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
 * DeferredRenderingManager renderer(device, allocator, mesh_manager,
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
        const vw::Model::MeshManager &mesh_manager,
        const vw::rt::RayTracedScene &ray_traced_scene,
        vk::Format depth_format = vk::Format::eD32Sfloat,
        vk::Format ao_format = vk::Format::eR32G32B32A32Sfloat,
        vk::Format light_format = vk::Format::eR16G16B16A16Sfloat)
        : m_device(std::move(device))
        , m_allocator(std::move(allocator))
        , m_ray_traced_scene(ray_traced_scene) {

        // Create functional passes - each owns its output images lazily
        m_zpass = std::make_unique<ZPass>(m_device, m_allocator, depth_format);

        m_color_pass = std::make_unique<ColorPass>(m_device, m_allocator,
                                                   mesh_manager, depth_format);

        m_ao_pass = std::make_unique<AmbientOcclusionPass>(
            m_device, m_allocator, ray_traced_scene.tlas_handle(), ao_format);

        m_sky_pass = std::make_unique<SkyPass>(m_device, m_allocator,
                                               light_format, depth_format);

        m_sun_light_pass = std::make_unique<SunLightPass>(
            m_device, m_allocator, ray_traced_scene.tlas_handle(),
            light_format);
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
     * @param sun_angle Sun angle in degrees above horizon (90 = zenith)
     * @param ao_radius AO sampling radius (default: 100.0)
     * @return The final light image view (can be blitted to swapchain)
     */
    template <typename UBOType>
    std::shared_ptr<const vw::ImageView> execute(
        vk::CommandBuffer cmd, vw::Barrier::ResourceTracker &tracker,
        vw::Width width, vw::Height height, size_t frame_index,
        const vw::Buffer<UBOType, true, vw::UniformBufferUsage> &uniform_buffer,
        float sun_angle = 90.0f, float ao_radius = 100.0f) {

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
            cmd, tracker, width, height, frame_index, depth_view, sun_angle,
            ubo_data.inverseViewProj);

        // 5. Sun Light Pass: add sun lighting with ray-traced shadows and AO
        m_sun_light_pass->execute(cmd, tracker,
                                  light_view,       // from SkyPass
                                  depth_view,       // from ZPass
                                  gbuffer.color,    // albedo from ColorPass
                                  gbuffer.position, // from ColorPass
                                  gbuffer.normal,   // from ColorPass
                                  ao_view,          // from AO Pass
                                  sun_angle);

        // Return AO buffer directly to visualize progressive accumulation
        return light_view;
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
    SkyPass &sky_pass() { return *m_sky_pass; }
    const SkyPass &sky_pass() const { return *m_sky_pass; }

    /** @brief Access the Sun Light Pass for custom usage */
    SunLightPass &sun_light_pass() { return *m_sun_light_pass; }
    const SunLightPass &sun_light_pass() const { return *m_sun_light_pass; }

    /** @brief Reset progressive state (call on resize) */
    void reset() { m_ao_pass->reset_accumulation(); }

  private:
    std::shared_ptr<vw::Device> m_device;
    std::shared_ptr<vw::Allocator> m_allocator;
    const vw::rt::RayTracedScene &m_ray_traced_scene;

    // Functional passes (each owns its output images lazily)
    std::unique_ptr<ZPass> m_zpass;
    std::unique_ptr<ColorPass> m_color_pass;
    std::unique_ptr<AmbientOcclusionPass> m_ao_pass;
    std::unique_ptr<SkyPass> m_sky_pass;
    std::unique_ptr<SunLightPass> m_sun_light_pass;
};
