#pragma once

#include "AmbientOcclusionPass.h"
#include "ColorPass.h"
#include "ZPass.h"
#include <memory>
#include <VulkanWrapper/Memory/Buffer.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/Model/Scene.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>
#include <VulkanWrapper/Vulkan/Device.h>

/**
 * @brief Manages deferred rendering passes with functional API and lazy allocation
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
        vk::Format ao_format = vk::Format::eR8G8B8A8Unorm)
        : m_device(std::move(device))
        , m_allocator(std::move(allocator))
        , m_ray_traced_scene(ray_traced_scene) {

        // Create functional passes - each owns its output images lazily
        m_zpass = std::make_unique<ZPass>(m_device, m_allocator, depth_format);

        m_color_pass = std::make_unique<ColorPass>(
            m_device, m_allocator, mesh_manager, depth_format);

        m_ao_pass = std::make_unique<AmbientOcclusionPass>(
            m_device, m_allocator, ray_traced_scene.tlas_handle(), ao_format);
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
     * @param num_ao_samples Number of AO samples (default: 32)
     * @param ao_radius AO sampling radius (default: 100.0)
     * @return The final AO image view (can be blitted to swapchain)
     */
    template <typename UBOType>
    std::shared_ptr<const vw::ImageView>
    execute(vk::CommandBuffer cmd, vw::Barrier::ResourceTracker &tracker,
            vw::Width width, vw::Height height, size_t frame_index,
            const vw::Buffer<UBOType, true, vw::UniformBufferUsage>
                &uniform_buffer,
            int32_t num_ao_samples = 32, float ao_radius = 100.0f) {

        const auto &scene = m_ray_traced_scene.scene();

        // 1. Z-Pass: depth prepass
        auto depth_view = m_zpass->execute(cmd, tracker, width, height,
                                           frame_index, scene, uniform_buffer);

        // 2. Color Pass: G-Buffer fill
        auto gbuffer = m_color_pass->execute(cmd, tracker, width, height,
                                             frame_index, scene, depth_view,
                                             uniform_buffer);

        // 3. AO Pass: screen-space ambient occlusion
        auto ao_view = m_ao_pass->execute(cmd, tracker, width, height,
                                          frame_index, gbuffer.position,
                                          gbuffer.normal, gbuffer.tangent,
                                          gbuffer.bitangent, num_ao_samples,
                                          ao_radius);

        return ao_view;
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

  private:
    std::shared_ptr<vw::Device> m_device;
    std::shared_ptr<vw::Allocator> m_allocator;
    const vw::rt::RayTracedScene &m_ray_traced_scene;

    // Functional passes (each owns its output images lazily)
    std::unique_ptr<ZPass> m_zpass;
    std::unique_ptr<ColorPass> m_color_pass;
    std::unique_ptr<AmbientOcclusionPass> m_ao_pass;
};
