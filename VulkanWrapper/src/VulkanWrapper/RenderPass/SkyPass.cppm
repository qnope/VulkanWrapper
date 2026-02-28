export module vw.renderpass:sky_pass;
import std3rd;
import vulkan3rd;
import glm3rd;
import :screen_space_pass;
import :sky_parameters;
import vw.vulkan;
import vw.memory;
import vw.sync;

export namespace vw {

enum class SkyPassSlot { Light };

/**
 * @brief Functional Sky pass with lazy image allocation
 *
 * This pass lazily allocates its light output image on first execute() call.
 * Images are cached by (width, height, frame_index) and reused on subsequent
 * calls. The sky is rendered where depth == 1.0 (far plane).
 *
 * Shaders are compiled at runtime from GLSL source files using ShaderCompiler.
 */
class SkyPass : public ScreenSpacePass<SkyPassSlot> {
  public:
    // Combined push constants: SkyParametersGPU + inverseViewProj
    struct PushConstants {
        SkyParametersGPU sky;      // Full sky parameters
        glm::mat4 inverseViewProj; // inverse(projection * view)
    };

    /**
     * @brief Construct a SkyPass with shaders loaded from files
     */
    SkyPass(std::shared_ptr<Device> device,
            std::shared_ptr<Allocator> allocator,
            const std::filesystem::path &shader_dir,
            vk::Format light_format = vk::Format::eR32G32B32A32Sfloat,
            vk::Format depth_format = vk::Format::eD32Sfloat);

    /**
     * @brief Execute the sky rendering pass
     */
    std::shared_ptr<const ImageView>
    execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
            Width width, Height height, size_t frame_index,
            std::shared_ptr<const ImageView> depth_view,
            const SkyParameters &sky_params,
            const glm::mat4 &inverse_view_proj);

  private:
    std::shared_ptr<const Pipeline>
    create_pipeline(const std::filesystem::path &shader_dir);

    vk::Format m_light_format;
    vk::Format m_depth_format;
    std::shared_ptr<const Pipeline> m_pipeline;
};

} // namespace vw
