export module vw.renderpass:tone_mapping_pass;
import std3rd;
import vulkan3rd;
import :screen_space_pass;
import vw.vulkan;
import vw.memory;
import vw.sync;
import vw.descriptors;
import vw.pipeline;

export namespace vw {

/**
 * @brief Tone mapping operators supported by ToneMappingPass
 */
enum class ToneMappingOperator : int32_t {
    ACES = 0,             ///< Academy Color Encoding System (default)
    Reinhard = 1,         ///< Simple Reinhard: L / (1 + L)
    ReinhardExtended = 2, ///< Reinhard with white point control
    Uncharted2 = 3,       ///< Hable filmic curve
    Neutral = 4           ///< Linear + clamp (no tone mapping)
};

// Empty slot enum - ToneMappingPass renders directly to swapchain
enum class ToneMappingPassSlot {};

/**
 * @brief Functional Tone Mapping pass for HDR to LDR conversion
 */
class ToneMappingPass : public ScreenSpacePass<ToneMappingPassSlot> {
  public:
    /**
     * @brief Push constants for tone mapping configuration
     */
    struct PushConstants {
        float exposure;           ///< EV multiplier (default: 1.0)
        int32_t operator_id;      ///< ToneMappingOperator enum value
        float white_point;        ///< For Reinhard Extended (default: 4.0)
        float luminance_scale;    ///< Factor to de-normalize HDR values
        float indirect_intensity; ///< Multiplier for indirect light (0.0 = off)
    };

    /// Default luminance scale: 1.0 means no de-normalization
    static constexpr float DEFAULT_LUMINANCE_SCALE = 1.0f;

    /**
     * @brief Construct a ToneMappingPass with shaders loaded from files
     */
    ToneMappingPass(std::shared_ptr<Device> device,
                    std::shared_ptr<Allocator> allocator,
                    const std::filesystem::path &shader_dir,
                    vk::Format output_format = vk::Format::eB8G8R8A8Srgb);

    /**
     * @brief Execute tone mapping to a provided output image view
     */
    void execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
                 std::shared_ptr<const ImageView> output_view,
                 std::shared_ptr<const ImageView> sky_view,
                 std::shared_ptr<const ImageView> direct_light_view,
                 std::shared_ptr<const ImageView> indirect_view = nullptr,
                 float indirect_intensity = 0.0f,
                 ToneMappingOperator tone_operator = ToneMappingOperator::ACES,
                 float exposure = 1.0f, float white_point = 4.0f,
                 float luminance_scale = DEFAULT_LUMINANCE_SCALE);

    /**
     * @brief Execute with lazy-allocated output image
     */
    std::shared_ptr<const ImageView>
    execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
            Width width, Height height, size_t frame_index,
            std::shared_ptr<const ImageView> sky_view,
            std::shared_ptr<const ImageView> direct_light_view,
            std::shared_ptr<const ImageView> indirect_view = nullptr,
            float indirect_intensity = 0.0f,
            ToneMappingOperator tone_operator = ToneMappingOperator::ACES,
            float exposure = 1.0f, float white_point = 4.0f,
            float luminance_scale = DEFAULT_LUMINANCE_SCALE);

    ToneMappingOperator get_operator() const { return m_current_operator; }
    void set_operator(ToneMappingOperator op) { m_current_operator = op; }
    float get_exposure() const { return m_exposure; }
    void set_exposure(float exposure) { m_exposure = exposure; }
    float get_white_point() const { return m_white_point; }
    void set_white_point(float white_point) { m_white_point = white_point; }

  private:
    struct CompiledShaders {
        std::shared_ptr<const ShaderModule> vertex;
        std::shared_ptr<const ShaderModule> fragment;
    };

    CompiledShaders compile_shaders(const std::filesystem::path &shader_dir);
    DescriptorPool create_descriptor_pool(CompiledShaders shaders);
    void create_black_fallback_image();

    vk::Format m_output_format;

    // Default parameters
    ToneMappingOperator m_current_operator = ToneMappingOperator::ACES;
    float m_exposure = 1.0f;
    float m_white_point = 4.0f;

    // Resources (order matters! m_pipeline must be before m_descriptor_pool)
    std::shared_ptr<const Sampler> m_sampler;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<const Pipeline> m_pipeline;
    DescriptorPool m_descriptor_pool;

    // Fallback 1x1 black image when indirect_view is nullptr
    std::shared_ptr<const Image> m_black_image;
    std::shared_ptr<const ImageView> m_black_image_view;
};

} // namespace vw
