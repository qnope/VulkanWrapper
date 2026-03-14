#pragma once

#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Pipeline/MeshRenderer.h"
#include "VulkanWrapper/Random/NoiseTexture.h"
#include "VulkanWrapper/Random/RandomSamplingBuffer.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"
#include "VulkanWrapper/RenderPass/SkyParameters.h"
#include <filesystem>

namespace vw {

class BufferBase;

namespace rt {
class RayTracedScene;
} // namespace rt

namespace Model::Material {
class BindlessMaterialManager;
} // namespace Model::Material

/// Format configuration for G-Buffer attachments
struct DirectLightPassFormats {
    vk::Format albedo = vk::Format::eR8G8B8A8Unorm;
    vk::Format normal = vk::Format::eR32G32B32A32Sfloat;
    vk::Format tangent = vk::Format::eR32G32B32A32Sfloat;
    vk::Format bitangent =
        vk::Format::eR32G32B32A32Sfloat;
    vk::Format position =
        vk::Format::eR32G32B32A32Sfloat;
    vk::Format direct_light =
        vk::Format::eR16G16B16A16Sfloat;
    vk::Format indirect_ray =
        vk::Format::eR32G32B32A32Sfloat;
    vk::Format depth = vk::Format::eD32Sfloat;
};

/**
 * @brief G-Buffer fill pass with per-fragment direct sun
 *        lighting
 *
 * This pass lazily allocates its G-Buffer images on first
 * execute() call. It computes direct sun lighting
 * per-fragment using ray queries for shadows, producing a
 * DirectLight attachment as part of the G-Buffer.
 *
 * Inputs: Slot::Depth (from ZPass)
 * Outputs: Slot::Albedo, Slot::Normal, Slot::Tangent,
 *          Slot::Bitangent, Slot::Position,
 *          Slot::DirectLight, Slot::IndirectRay
 */
class DirectLightPass : public RenderPass {
  public:
    using Formats = DirectLightPassFormats;

    DirectLightPass(
        std::shared_ptr<Device> device,
        std::shared_ptr<Allocator> allocator,
        const std::filesystem::path &shader_dir,
        const rt::RayTracedScene &ray_traced_scene,
        Model::Material::BindlessMaterialManager
            &material_manager,
        Formats formats = {});

    std::vector<Slot> input_slots() const override;
    std::vector<Slot> output_slots() const override;

    std::string_view name() const override {
        return "DirectLightPass";
    }

    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker &tracker,
                 Width width, Height height,
                 size_t frame_index) override;

    /// Set the uniform buffer containing view/projection
    /// matrices
    void set_uniform_buffer(const BufferBase &ubo);

    /// Set sky and sun parameters for direct lighting
    void set_sky_parameters(const SkyParameters &params);

    /// Set the camera position for view direction
    /// computation
    void set_camera_position(const glm::vec3 &pos);

    /// Set the frame count for temporal sampling
    void set_frame_count(uint32_t count);

  private:
    Formats m_formats;
    const rt::RayTracedScene *m_ray_traced_scene;
    Model::Material::BindlessMaterialManager
        *m_material_manager;

    // Pipelines (one per material type via MeshRenderer)
    MeshRenderer m_mesh_renderer;

    // Descriptor resources
    std::shared_ptr<DescriptorSetLayout>
        m_descriptor_layout;
    DescriptorPool m_descriptor_pool;

    // Sky parameters UBO
    Buffer<SkyParametersGPU, true, UniformBufferUsage>
        m_sky_params_buffer;

    // Random sampling resources
    DualRandomSampleBuffer m_hemisphere_samples;
    std::unique_ptr<NoiseTexture> m_noise_texture;

    // Per-frame state
    const BufferBase *m_uniform_buffer = nullptr;
    SkyParameters m_sky_params =
        SkyParameters::create_earth_sun(45.0f);
    glm::vec3 m_camera_pos{0.f};
    uint32_t m_frame_count = 0;
};

} // namespace vw
