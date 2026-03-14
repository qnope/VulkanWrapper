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
 * Each operator provides a different look and feel for HDR to LDR
 * conversion.
 */
enum class ToneMappingOperator : int32_t {
    ACES = 0,             ///< Academy Color Encoding System
    Reinhard = 1,         ///< Simple Reinhard: L / (1 + L)
    ReinhardExtended = 2, ///< Reinhard with white point control
    Uncharted2 = 3,       ///< Hable filmic curve
    Neutral = 4           ///< Linear + clamp (no tone mapping)
};

/**
 * @brief Functional Tone Mapping pass for HDR to LDR conversion
 *
 * This pass applies tone mapping to convert HDR radiance values to
 * displayable LDR output. It supports multiple tone mapping operators
 * and can render either to the swapchain directly or to an
 * internally-allocated image.
 *
 * The pass expects HDR light buffers that already contain the final
 * lit color (radiance). It applies exposure adjustment and tone
 * mapping to produce displayable LDR output.
 *
 * Inputs: Slot::Sky, Slot::DirectLight, optionally
 *         Slot::IndirectLight
 * Outputs: Slot::ToneMapped
 *
 * @note Gamma correction is NOT applied by this pass. Use sRGB
 *       output formats (e.g., eB8G8R8A8Srgb) for hardware gamma.
 */
class ToneMappingPass : public ScreenSpacePass {
  public:
    /**
     * @brief Push constants for tone mapping configuration
     */
    struct PushConstants {
        float exposure;
        int32_t operator_id;
        float white_point;
        float luminance_scale;
        float indirect_intensity;
    };

    /// Default luminance scale: 1.0 means no de-normalization
    static constexpr float DEFAULT_LUMINANCE_SCALE = 1.0f;

    /**
     * @brief Construct a ToneMappingPass with shaders loaded from
     *        files
     *
     * @param device Vulkan device
     * @param allocator Memory allocator
     * @param shader_dir Path to the shader directory containing
     *                   fullscreen.vert and tonemap.frag
     * @param output_format Output image format
     * @param indirect_enabled Whether IndirectLight is an input
     */
    ToneMappingPass(
        std::shared_ptr<Device> device,
        std::shared_ptr<Allocator> allocator,
        const std::filesystem::path &shader_dir,
        vk::Format output_format = vk::Format::eB8G8R8A8Srgb,
        bool indirect_enabled = true);

    // -- Slot introspection --
    std::vector<Slot> input_slots() const override;
    std::vector<Slot> output_slots() const override {
        return {Slot::ToneMapped};
    }

    // -- Unified execute --
    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker &tracker, Width width,
                 Height height, size_t frame_index) override;

    /**
     * @brief Execute tone mapping to a provided output image view
     *
     * This variant renders directly to an external image (e.g.,
     * swapchain). Non-virtual overload for backward compatibility.
     */
    void execute_to_view(
        vk::CommandBuffer cmd,
        Barrier::ResourceTracker &tracker,
        std::shared_ptr<const ImageView> output_view,
        std::shared_ptr<const ImageView> sky_view,
        std::shared_ptr<const ImageView> direct_light_view,
        std::shared_ptr<const ImageView> indirect_view =
            nullptr,
        float indirect_intensity = 0.0f,
        ToneMappingOperator tone_operator =
            ToneMappingOperator::ACES,
        float exposure = 1.0f, float white_point = 4.0f,
        float luminance_scale = DEFAULT_LUMINANCE_SCALE);

    std::string_view name() const override {
        return "ToneMappingPass";
    }

    // -- Getters / setters --
    ToneMappingOperator get_operator() const {
        return m_current_operator;
    }
    void set_operator(ToneMappingOperator op) {
        m_current_operator = op;
    }

    float get_exposure() const { return m_exposure; }
    void set_exposure(float exposure) {
        m_exposure = exposure;
    }

    float get_white_point() const { return m_white_point; }
    void set_white_point(float white_point) {
        m_white_point = white_point;
    }

    float get_luminance_scale() const {
        return m_luminance_scale;
    }
    void set_luminance_scale(float scale) {
        m_luminance_scale = scale;
    }

    float get_indirect_intensity() const {
        return m_indirect_intensity;
    }
    void set_indirect_intensity(float intensity) {
        m_indirect_intensity = intensity;
    }

    bool is_indirect_enabled() const {
        return m_indirect_enabled;
    }
    void set_indirect_enabled(bool enabled) {
        m_indirect_enabled = enabled;
    }

  private:
    struct CompiledShaders {
        std::shared_ptr<const ShaderModule> vertex;
        std::shared_ptr<const ShaderModule> fragment;
    };

    CompiledShaders
    compile_shaders(const std::filesystem::path &shader_dir);
    DescriptorPool
    create_descriptor_pool(CompiledShaders shaders);
    void create_black_fallback_image();

    vk::Format m_output_format;
    bool m_indirect_enabled;

    // Default parameters
    ToneMappingOperator m_current_operator =
        ToneMappingOperator::ACES;
    float m_exposure = 1.0f;
    float m_white_point = 4.0f;
    float m_luminance_scale = DEFAULT_LUMINANCE_SCALE;
    float m_indirect_intensity = 0.0f;

    // Resources
    std::shared_ptr<const Sampler> m_sampler;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<const Pipeline> m_pipeline;
    DescriptorPool m_descriptor_pool;

    // Fallback 1x1 black image when indirect_view is not wired
    std::shared_ptr<const Image> m_black_image;
    std::shared_ptr<const ImageView> m_black_image_view;
};

} // namespace vw
