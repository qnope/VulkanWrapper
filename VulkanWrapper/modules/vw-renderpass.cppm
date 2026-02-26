module;
#include "VulkanWrapper/3rd_party.h"
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

export module vw:renderpass;
import :core;
import :utils;
import :vulkan;
import :synchronization;
import :image;
import :memory;
import :descriptors;
import :pipeline;
import :shader;
import :model;
import :raytracing;

export namespace vw {

// ---- CachedImage ----

struct CachedImage {
    std::shared_ptr<const Image> image;
    std::shared_ptr<const ImageView> view;
};

// ---- Subpass<SlotEnum> ----

template <typename SlotEnum> class Subpass {
  public:
    Subpass(std::shared_ptr<Device> device,
            std::shared_ptr<Allocator> allocator)
        : m_device(std::move(device))
        , m_allocator(std::move(allocator)) {}

    Subpass(const Subpass &) = delete;
    Subpass(Subpass &&) = delete;
    Subpass &operator=(Subpass &&) = delete;
    Subpass &operator=(const Subpass &) = delete;

    virtual ~Subpass() = default;

  protected:
    const CachedImage &get_or_create_image(SlotEnum slot, Width width,
                                           Height height, size_t frame_index,
                                           vk::Format format,
                                           vk::ImageUsageFlags usage) {
        ImageKey key{slot, static_cast<uint32_t>(width),
                     static_cast<uint32_t>(height), frame_index};

        auto it = m_image_cache.find(key);
        if (it != m_image_cache.end()) {
            return it->second;
        }

        std::erase_if(m_image_cache, [&](const auto &entry) {
            return entry.first.slot == slot &&
                   (entry.first.width != key.width ||
                    entry.first.height != key.height);
        });

        auto image =
            m_allocator->create_image_2D(width, height, false, format, usage);

        auto view = ImageViewBuilder(m_device, image)
                        .setImageType(vk::ImageViewType::e2D)
                        .build();

        auto [inserted_it, success] = m_image_cache.emplace(
            key, CachedImage{std::move(image), std::move(view)});

        return inserted_it->second;
    }

    std::shared_ptr<Device> m_device;
    std::shared_ptr<Allocator> m_allocator;

  private:
    struct ImageKey {
        SlotEnum slot;
        uint32_t width;
        uint32_t height;
        size_t frame_index;

        auto operator<=>(const ImageKey &) const = default;
    };

    std::map<ImageKey, CachedImage> m_image_cache;
};

// ---- ScreenSpacePass<SlotEnum> ----

template <typename SlotEnum> class ScreenSpacePass : public Subpass<SlotEnum> {
  public:
    ScreenSpacePass(std::shared_ptr<Device> device,
                    std::shared_ptr<Allocator> allocator)
        : Subpass<SlotEnum>(std::move(device), std::move(allocator)) {}

  protected:
    std::shared_ptr<const Sampler> create_default_sampler() {
        return SamplerBuilder(this->m_device).build();
    }

    void render_fullscreen(
        vk::CommandBuffer cmd, vk::Extent2D extent,
        const vk::RenderingAttachmentInfo &color_attachment,
        const vk::RenderingAttachmentInfo *depth_attachment,
        const Pipeline &pipeline,
        std::optional<DescriptorSet> descriptor_set = std::nullopt,
        const void *push_constants = nullptr, size_t push_constants_size = 0) {

        vk::RenderingInfo rendering_info =
            vk::RenderingInfo()
                .setRenderArea(vk::Rect2D({0, 0}, extent))
                .setLayerCount(1)
                .setColorAttachments(color_attachment);

        if (depth_attachment) {
            rendering_info.setPDepthAttachment(depth_attachment);
        }

        cmd.beginRendering(rendering_info);

        vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width),
                              static_cast<float>(extent.height), 0.0f, 1.0f);
        vk::Rect2D scissor({0, 0}, extent);
        cmd.setViewport(0, 1, &viewport);
        cmd.setScissor(0, 1, &scissor);

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle());

        if (descriptor_set) {
            auto descriptor_handle = descriptor_set->handle();
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   pipeline.layout().handle(), 0, 1,
                                   &descriptor_handle, 0, nullptr);
        }

        if (push_constants && push_constants_size > 0) {
            cmd.pushConstants(pipeline.layout().handle(),
                              vk::ShaderStageFlagBits::eFragment, 0,
                              push_constants_size, push_constants);
        }

        cmd.draw(4, 1, 0, 0);

        cmd.endRendering();
    }
};

std::shared_ptr<const Pipeline> create_screen_space_pipeline(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const ShaderModule> vertex_shader,
    std::shared_ptr<const ShaderModule> fragment_shader,
    std::shared_ptr<const DescriptorSetLayout> descriptor_set_layout,
    vk::Format color_format, vk::Format depth_format = vk::Format::eUndefined,
    vk::CompareOp depth_compare_op = vk::CompareOp::eAlways,
    std::vector<vk::PushConstantRange> push_constants = {},
    std::optional<ColorBlendConfig> blend = std::nullopt);

// ---- SkyParameters ----

struct SkyParametersGPU {
    glm::vec4 star_direction_and_constant;
    glm::vec4 star_color_and_solid_angle;
    glm::vec4 rayleigh_and_height_r;
    glm::vec4 mie_and_height_m;
    glm::vec4 ozone_and_height_o;
    glm::vec4 radii_and_efficiency;
};

static_assert(sizeof(SkyParametersGPU) == 96,
              "SkyParametersGPU must be 96 bytes");

struct SkyParameters {
    float star_constant;
    glm::vec3 star_direction;
    glm::vec3 star_color;
    float star_solid_angle;

    glm::vec3 rayleigh_coef;
    glm::vec3 mie_coef;
    glm::vec3 ozone_coef;

    float height_rayleigh;
    float height_mie;
    float height_ozone;

    float radius_planet;
    float radius_atmosphere;

    float luminous_efficiency;

    static glm::vec3 angle_to_direction(float angle_deg);
    static glm::vec3 temperature_to_color(float temperature_kelvin);
    static float angular_diameter_to_solid_angle(float angular_diameter_deg);
    static float compute_radiance(float solar_constant, float solid_angle);
    static SkyParameters create_earth_sun(float sun_angle_deg);

    glm::vec3 direction_to_star() const { return -star_direction; }

    float star_radiance() const {
        return compute_radiance(star_constant, star_solid_angle);
    }

    SkyParametersGPU to_gpu() const {
        return SkyParametersGPU{
            .star_direction_and_constant =
                glm::vec4(star_direction, star_constant),
            .star_color_and_solid_angle =
                glm::vec4(star_color, star_solid_angle),
            .rayleigh_and_height_r = glm::vec4(rayleigh_coef, height_rayleigh),
            .mie_and_height_m = glm::vec4(mie_coef, height_mie),
            .ozone_and_height_o = glm::vec4(ozone_coef, height_ozone),
            .radii_and_efficiency = glm::vec4(radius_planet, radius_atmosphere,
                                              luminous_efficiency, 0.0f)};
    }
};

static_assert(sizeof(SkyParameters) <= 128,
              "SkyParameters must fit in push constants");

// ---- SkyPass ----

enum class SkyPassSlot { Light };

class SkyPass : public ScreenSpacePass<SkyPassSlot> {
  public:
    struct PushConstants {
        SkyParametersGPU sky;
        glm::mat4 inverseViewProj;
    };

    SkyPass(std::shared_ptr<Device> device,
            std::shared_ptr<Allocator> allocator,
            const std::filesystem::path &shader_dir,
            vk::Format light_format = vk::Format::eR32G32B32A32Sfloat,
            vk::Format depth_format = vk::Format::eD32Sfloat);

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

// ---- ToneMappingPass ----

enum class ToneMappingOperator : int32_t {
    ACES = 0,
    Reinhard = 1,
    ReinhardExtended = 2,
    Uncharted2 = 3,
    Neutral = 4
};

enum class ToneMappingPassSlot {};

class ToneMappingPass : public ScreenSpacePass<ToneMappingPassSlot> {
  public:
    struct PushConstants {
        float exposure;
        int32_t operator_id;
        float white_point;
        float luminance_scale;
        float indirect_intensity;
    };

    static constexpr float DEFAULT_LUMINANCE_SCALE = 1.0f;

    ToneMappingPass(std::shared_ptr<Device> device,
                    std::shared_ptr<Allocator> allocator,
                    const std::filesystem::path &shader_dir,
                    vk::Format output_format = vk::Format::eB8G8R8A8Srgb);

    void execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
                 std::shared_ptr<const ImageView> output_view,
                 std::shared_ptr<const ImageView> sky_view,
                 std::shared_ptr<const ImageView> direct_light_view,
                 std::shared_ptr<const ImageView> indirect_view = nullptr,
                 float indirect_intensity = 0.0f,
                 ToneMappingOperator tone_operator = ToneMappingOperator::ACES,
                 float exposure = 1.0f, float white_point = 4.0f,
                 float luminance_scale = DEFAULT_LUMINANCE_SCALE);

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

    ToneMappingOperator m_current_operator = ToneMappingOperator::ACES;
    float m_exposure = 1.0f;
    float m_white_point = 4.0f;

    std::shared_ptr<const Sampler> m_sampler;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<const Pipeline> m_pipeline;
    DescriptorPool m_descriptor_pool;

    std::shared_ptr<const Image> m_black_image;
    std::shared_ptr<const ImageView> m_black_image_view;
};

// ---- IndirectLightPass ----

enum class IndirectLightPassSlot {
    Output
};

struct IndirectLightPushConstants {
    SkyParametersGPU sky;
    uint32_t frame_count;
    uint32_t width;
    uint32_t height;
};

static_assert(sizeof(IndirectLightPushConstants) <= 128,
              "IndirectLightPushConstants must fit in push constant limit");

class IndirectLightPass : public Subpass<IndirectLightPassSlot> {
  public:
    IndirectLightPass(std::shared_ptr<Device> device,
                      std::shared_ptr<Allocator> allocator,
                      const std::filesystem::path &shader_dir,
                      const rt::as::TopLevelAccelerationStructure &tlas,
                      const rt::GeometryReferenceBuffer &geometry_buffer,
                      Model::Material::BindlessMaterialManager &material_manager,
                      vk::Format output_format = vk::Format::eR32G32B32A32Sfloat);

    std::shared_ptr<const ImageView>
    execute(vk::CommandBuffer cmd, Barrier::ResourceTracker &tracker,
            Width width, Height height,
            std::shared_ptr<const ImageView> position_view,
            std::shared_ptr<const ImageView> normal_view,
            std::shared_ptr<const ImageView> albedo_view,
            std::shared_ptr<const ImageView> ao_view,
            std::shared_ptr<const ImageView> indirect_ray_view,
            const SkyParameters &sky_params);

    void reset_accumulation() { m_frame_count = 0; }

    uint32_t get_frame_count() const { return m_frame_count; }

  private:
    void create_pipeline_and_sbt(const std::filesystem::path &shader_dir);

    const rt::as::TopLevelAccelerationStructure *m_tlas;
    const rt::GeometryReferenceBuffer *m_geometry_buffer;
    Model::Material::BindlessMaterialManager *m_material_manager;
    vk::Format m_output_format;

    uint32_t m_frame_count = 0;

    std::shared_ptr<const Sampler> m_sampler;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_layout;
    std::unique_ptr<rt::RayTracingPipeline> m_pipeline;
    std::unique_ptr<rt::ShaderBindingTable> m_sbt;
    DescriptorPool m_descriptor_pool;

    std::shared_ptr<DescriptorSetLayout> m_texture_descriptor_layout;
    DescriptorPool m_texture_descriptor_pool;
};

// MeshRenderer is defined in :model partition

} // namespace vw
