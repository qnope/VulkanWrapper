export module vw.renderpass:indirect_light_pass;
import std3rd;
import vulkan3rd;
import :subpass;
import :sky_parameters;
import vw.vulkan;
import vw.memory;
import vw.sync;
import vw.descriptors;
import vw.raytracing;
import vw.model;

export namespace vw {

namespace Model::Material {} // namespace Model::Material

enum class IndirectLightPassSlot {
    Output // Single accumulation buffer (storage image for RT)
};

/**
 * @brief Push constants for IndirectLightPass
 */
struct IndirectLightPushConstants {
    SkyParametersGPU sky; // 96 bytes
    uint32_t frame_count; // 4 bytes
    uint32_t width;       // 4 bytes
    uint32_t height;      // 4 bytes
}; // 108 bytes total

static_assert(sizeof(IndirectLightPushConstants) <= 128,
              "IndirectLightPushConstants must fit in push constant limit");

/**
 * @brief Ray Tracing Indirect Light pass with progressive accumulation
 *
 * This pass computes indirect sky lighting using a ray tracing pipeline.
 */
class IndirectLightPass : public Subpass<IndirectLightPassSlot> {
  public:
    /**
     * @brief Construct an IndirectLightPass with shaders loaded from files
     */
    IndirectLightPass(
        std::shared_ptr<Device> device, std::shared_ptr<Allocator> allocator,
        const std::filesystem::path &shader_dir,
        const rt::as::TopLevelAccelerationStructure &tlas,
        const rt::GeometryReferenceBuffer &geometry_buffer,
        Model::Material::BindlessMaterialManager &material_manager,
        vk::Format output_format = vk::Format::eR32G32B32A32Sfloat);

    /**
     * @brief Execute the indirect light pass with progressive accumulation
     */
    std::shared_ptr<const ImageView>
    execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
            Width width, Height height,
            std::shared_ptr<const ImageView> position_view,
            std::shared_ptr<const ImageView> normal_view,
            std::shared_ptr<const ImageView> albedo_view,
            std::shared_ptr<const ImageView> ao_view,
            std::shared_ptr<const ImageView> indirect_ray_view,
            const SkyParameters &sky_params);

    /**
     * @brief Reset progressive accumulation
     */
    void reset_accumulation() { m_frame_count = 0; }

    /**
     * @brief Get the current frame count for progressive accumulation
     */
    uint32_t get_frame_count() const { return m_frame_count; }

  private:
    void create_pipeline_and_sbt(const std::filesystem::path &shader_dir);

    const rt::as::TopLevelAccelerationStructure *m_tlas;
    const rt::GeometryReferenceBuffer *m_geometry_buffer;
    Model::Material::BindlessMaterialManager *m_material_manager;
    vk::Format m_output_format;

    // Progressive accumulation state
    uint32_t m_frame_count = 0;

    // Resources (order matters for destruction!)
    std::shared_ptr<const Sampler> m_sampler;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_layout;
    std::unique_ptr<rt::RayTracingPipeline> m_pipeline;
    std::unique_ptr<rt::ShaderBindingTable> m_sbt;
    DescriptorPool m_descriptor_pool;

    // Texture descriptor resources for per-material hit shaders
    std::shared_ptr<DescriptorSetLayout> m_texture_descriptor_layout;
    DescriptorPool m_texture_descriptor_pool;
};

} // namespace vw
