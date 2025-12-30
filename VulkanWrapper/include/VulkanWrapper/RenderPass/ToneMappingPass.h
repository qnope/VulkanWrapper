#pragma once

#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include <filesystem>

namespace vw {

/**
 * @brief Tone mapping operators supported by ToneMappingPass
 *
 * Each operator provides a different look and feel for HDR to LDR conversion.
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
 *
 * This pass applies tone mapping to convert HDR radiance values to displayable
 * LDR output. It supports multiple tone mapping operators and can render
 * either to the swapchain directly or to an internally-allocated image.
 *
 * The pass expects an HDR light buffer that already contains the final lit
 * color (radiance). It applies exposure adjustment and tone mapping to produce
 * displayable LDR output.
 *
 * @note Gamma correction is NOT applied by this pass. Use sRGB output formats
 *       (e.g., eB8G8R8A8Srgb) to have Vulkan handle gamma encoding in hardware.
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
    /// Use this when shaders already output normalized values (divided by
    /// LUMINANCE_SCALE). Only set to 10000.0 if shaders output raw physical
    /// luminance values.
    static constexpr float DEFAULT_LUMINANCE_SCALE = 1.0f;

    /**
     * @brief Construct a ToneMappingPass with shaders loaded from files
     *
     * Shaders are compiled at runtime from GLSL source files.
     *
     * @param device Vulkan device
     * @param allocator Memory allocator
     * @param shader_dir Path to the shader directory containing fullscreen.vert
     *                   and tonemap.frag
     * @param output_format Output image format (e.g., swapchain format)
     */
    ToneMappingPass(std::shared_ptr<Device> device,
                    std::shared_ptr<Allocator> allocator,
                    const std::filesystem::path &shader_dir,
                    vk::Format output_format = vk::Format::eB8G8R8A8Srgb);

    /**
     * @brief Execute tone mapping to a provided output image view
     *
     * This variant renders directly to an external image (e.g., swapchain).
     *
     * @param cmd Command buffer to record into
     * @param tracker Resource tracker for barrier management
     * @param output_view Output image view to render into
     * @param hdr_view HDR radiance buffer (direct light from lighting passes)
     * @param indirect_view Optional indirect light buffer (nullptr = no indirect)
     * @param indirect_intensity Multiplier for indirect light (0.0 = disabled)
     * @param tone_operator Tone mapping operator to use
     * @param exposure Exposure multiplier (EV)
     * @param white_point White point for Reinhard Extended operator
     * @param luminance_scale Factor to de-normalize HDR values
     */
    void execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
                 std::shared_ptr<const ImageView> output_view,
                 std::shared_ptr<const ImageView> hdr_view,
                 std::shared_ptr<const ImageView> indirect_view = nullptr,
                 float indirect_intensity = 0.0f,
                 ToneMappingOperator tone_operator = ToneMappingOperator::ACES,
                 float exposure = 1.0f, float white_point = 4.0f,
                 float luminance_scale = DEFAULT_LUMINANCE_SCALE);

    /**
     * @brief Execute with lazy-allocated output image
     *
     * @param cmd Command buffer to record into
     * @param tracker Resource tracker for barrier management
     * @param width Output width
     * @param height Output height
     * @param frame_index Frame index for multi-buffering
     * @param hdr_view HDR radiance buffer (direct light)
     * @param indirect_view Optional indirect light buffer (nullptr = no indirect)
     * @param indirect_intensity Multiplier for indirect light (0.0 = disabled)
     * @param tone_operator Tone mapping operator
     * @param exposure Exposure multiplier
     * @param white_point White point for Reinhard Extended
     * @param luminance_scale Factor to de-normalize HDR values
     * @return The tone-mapped output image view
     */
    std::shared_ptr<const ImageView>
    execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
            Width width, Height height, size_t frame_index,
            std::shared_ptr<const ImageView> hdr_view,
            std::shared_ptr<const ImageView> indirect_view = nullptr,
            float indirect_intensity = 0.0f,
            ToneMappingOperator tone_operator = ToneMappingOperator::ACES,
            float exposure = 1.0f, float white_point = 4.0f,
            float luminance_scale = DEFAULT_LUMINANCE_SCALE);

    /**
     * @brief Get the current tone mapping operator
     */
    ToneMappingOperator get_operator() const { return m_current_operator; }

    /**
     * @brief Set the default tone mapping operator
     */
    void set_operator(ToneMappingOperator op) { m_current_operator = op; }

    /**
     * @brief Get the current exposure value
     */
    float get_exposure() const { return m_exposure; }

    /**
     * @brief Set the exposure value
     */
    void set_exposure(float exposure) { m_exposure = exposure; }

    /**
     * @brief Get the current white point value
     */
    float get_white_point() const { return m_white_point; }

    /**
     * @brief Set the white point value (for Reinhard Extended)
     */
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
