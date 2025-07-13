#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/AccelerationStructure/AccelerationStructure.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/Framebuffer.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Pipeline/RayTracingPipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/Image/Sampler.h"

struct SunLightingPassTag {};
const auto sun_lighting_pass_tag = vw::create_subpass_tag<SunLightingPassTag>();

class SunLightingPass : public vw::Subpass {
  public:
    struct CameraUBO {
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 model;
    };

    struct SunUBO {
        glm::vec3 sunDirection;
        float sunIntensity;
        glm::vec3 sunColor;
        float padding;
    };

    SunLightingPass(const vw::Device &device, const vw::Allocator &allocator,
                    vw::Width width, vw::Height height, 
                    const glm::mat4 &projection, const glm::mat4 &view,
                    const glm::mat4 &model, const vw::AccelerationStructure::TopLevelAccelerationStructure &tlas,
                    const std::shared_ptr<const vw::ImageView> &gbufferPosition,
                    const std::shared_ptr<const vw::ImageView> &gbufferNormal,
                    const std::shared_ptr<const vw::ImageView> &gbufferAlbedo,
                    const std::shared_ptr<const vw::ImageView> &gbufferRoughness,
                    const std::shared_ptr<const vw::ImageView> &gbufferMetallic)
        : m_device{device}
        , m_allocator{allocator}
        , m_width{width}
        , m_height{height}
        , m_tlas{tlas}
        , m_gbufferPosition{gbufferPosition}
        , m_gbufferNormal{gbufferNormal}
        , m_gbufferAlbedo{gbufferAlbedo}
        , m_gbufferRoughness{gbufferRoughness}
        , m_gbufferMetallic{gbufferMetallic}
        , m_sbt_buffer{m_allocator.create_buffer<uint8_t, true, vw::StagingBufferUsage>(1024)} {
        
        const CameraUBO cameraUbo{projection, view, model};
        m_cameraUbo.copy(std::span{&cameraUbo, 1}, 0);

        const SunUBO sunUbo{
            glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f)), // sun direction
            1.0f, // intensity
            glm::vec3(1.0f, 0.95f, 0.8f), // sun color (warm white)
            0.0f // padding
        };
        m_sunUbo.copy(std::span{&sunUbo, 1}, 0);

        vw::DescriptorAllocator alloc;
        alloc.add_uniform_buffer(0, m_cameraUbo.handle(), 0, sizeof(CameraUBO));
        alloc.add_uniform_buffer(1, m_sunUbo.handle(), 0, sizeof(SunUBO));
        alloc.add_acceleration_structure(2, tlas.handle());
        alloc.add_combined_image(3, vw::CombinedImage{m_gbufferPosition, m_sampler});
        alloc.add_combined_image(4, vw::CombinedImage{m_gbufferNormal, m_sampler});
        alloc.add_combined_image(5, vw::CombinedImage{m_gbufferAlbedo, m_sampler});
        alloc.add_combined_image(6, vw::CombinedImage{m_gbufferRoughness, m_sampler});
        alloc.add_combined_image(7, vw::CombinedImage{m_gbufferMetallic, m_sampler});
        m_descriptor_set = m_descriptor_pool.allocate_set(alloc);
    }

    void execute(vk::CommandBuffer cmd_buffer,
                 const vw::Framebuffer &) const noexcept override {
        cmd_buffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,
                                m_pipeline->handle());
        cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                                      m_pipeline->layout().handle(), 0,
                                      m_descriptor_set, nullptr);
        
        // Dispatch ray tracing
        const uint32_t width = static_cast<uint32_t>(m_width);
        const uint32_t height = static_cast<uint32_t>(m_height);
        cmd_buffer.traceRaysKHR(m_raygen_sbt_region, m_miss_sbt_region, 
                                 m_hit_sbt_region, m_callable_sbt_region,
                                 width, height, 1);
    }

    const std::vector<vk::AttachmentReference2> &
    color_attachments() const noexcept override {
        static const std::vector<vk::AttachmentReference2> color_attachments = {
            vk::AttachmentReference2(5,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageAspectFlagBits::eColor)};
        return color_attachments;
    }

    vw::SubpassDependencyMask input_dependencies() const noexcept override {
        vw::SubpassDependencyMask mask;
        mask.access = vk::AccessFlagBits::eColorAttachmentRead;
        mask.stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        return mask;
    }

    vw::SubpassDependencyMask output_dependencies() const noexcept override {
        vw::SubpassDependencyMask mask;
        mask.access = vk::AccessFlagBits::eColorAttachmentWrite;
        mask.stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        return mask;
    }

    auto *get_camera_ubo() { return &m_cameraUbo; }
    auto *get_sun_ubo() { return &m_sunUbo; }

  protected:
    void initialize(const vw::RenderPass &render_pass) override {
        auto raygen = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/raytracing/sun_lighting.spv");
        auto closest_hit = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/raytracing/closest_hit.spv");
        auto miss = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/raytracing/miss.spv");

        auto pipelineLayout = vw::PipelineLayoutBuilder(m_device)
                                  .with_descriptor_set_layout(m_layout)
                                  .build();

        m_pipeline.emplace(
            vw::RayTracingPipelineBuilder(m_device, std::move(pipelineLayout))
                .add_ray_generation_shader(std::move(raygen))
                .add_closest_hit_shader(std::move(closest_hit))
                .add_miss_shader(std::move(miss))
                .build());

        // Create shader binding table
        createShaderBindingTable();
    }

  private:
    void createShaderBindingTable() {
        const auto handle_size = m_pipeline->get_shader_group_handle_size();
        const auto group_count = 3; // raygen + closest_hit + miss
        
        const auto sbt_size = handle_size * group_count;
        m_sbt_buffer = m_allocator.create_buffer<uint8_t, true, vw::StagingBufferUsage>(sbt_size);
        
        // Get shader group handles
        const auto handles = m_pipeline->get_shader_group_handles();
        m_sbt_buffer.copy(std::span{handles.data(), handles.size()}, 0);
        
        // Create SBT regions
        m_raygen_sbt_region = vk::StridedDeviceAddressRegionKHR()
            .setDeviceAddress(m_sbt_buffer.device_address())
            .setStride(handle_size)
            .setSize(handle_size);
            
        m_miss_sbt_region = vk::StridedDeviceAddressRegionKHR()
            .setDeviceAddress(m_sbt_buffer.device_address() + handle_size)
            .setStride(handle_size)
            .setSize(handle_size);
            
        m_hit_sbt_region = vk::StridedDeviceAddressRegionKHR()
            .setDeviceAddress(m_sbt_buffer.device_address() + 2 * handle_size)
            .setStride(handle_size)
            .setSize(handle_size);
            
        m_callable_sbt_region = vk::StridedDeviceAddressRegionKHR()
            .setDeviceAddress(0)
            .setStride(0)
            .setSize(0);
    }

    const vw::Device &m_device;
    const vw::Allocator &m_allocator;
    vw::Width m_width;
    vw::Height m_height;
    const vw::AccelerationStructure::TopLevelAccelerationStructure &m_tlas;
    const std::shared_ptr<const vw::ImageView> &m_gbufferPosition;
    const std::shared_ptr<const vw::ImageView> &m_gbufferNormal;
    const std::shared_ptr<const vw::ImageView> &m_gbufferAlbedo;
    const std::shared_ptr<const vw::ImageView> &m_gbufferRoughness;
    const std::shared_ptr<const vw::ImageView> &m_gbufferMetallic;
    
    vw::Buffer<CameraUBO, true, vw::UniformBufferUsage> m_cameraUbo =
        m_allocator.create_buffer<CameraUBO, true, vw::UniformBufferUsage>(1);
    vw::Buffer<SunUBO, true, vw::UniformBufferUsage> m_sunUbo =
        m_allocator.create_buffer<SunUBO, true, vw::UniformBufferUsage>(1);
    vw::Buffer<uint8_t, true, vw::StagingBufferUsage> m_sbt_buffer;
    
    std::shared_ptr<const vw::DescriptorSetLayout> m_layout =
        vw::DescriptorSetLayoutBuilder(m_device)
            .with_uniform_buffer(vk::ShaderStageFlagBits::eRaygenKHR, 1)
            .with_uniform_buffer(vk::ShaderStageFlagBits::eRaygenKHR, 1)
            .with_acceleration_structure(vk::ShaderStageFlagBits::eRaygenKHR)
            .with_combined_image_sampler(vk::ShaderStageFlagBits::eRaygenKHR, 1)
            .with_combined_image_sampler(vk::ShaderStageFlagBits::eRaygenKHR, 1)
            .with_combined_image_sampler(vk::ShaderStageFlagBits::eRaygenKHR, 1)
            .with_combined_image_sampler(vk::ShaderStageFlagBits::eRaygenKHR, 1)
            .with_combined_image_sampler(vk::ShaderStageFlagBits::eRaygenKHR, 1)
            .build();

    mutable vw::DescriptorPool m_descriptor_pool =
        vw::DescriptorPoolBuilder(m_device, m_layout).build();
    std::optional<vw::RayTracingPipeline> m_pipeline;
    vk::DescriptorSet m_descriptor_set;
    
    vk::StridedDeviceAddressRegionKHR m_raygen_sbt_region;
    vk::StridedDeviceAddressRegionKHR m_miss_sbt_region;
    vk::StridedDeviceAddressRegionKHR m_hit_sbt_region;
    vk::StridedDeviceAddressRegionKHR m_callable_sbt_region;

    std::shared_ptr<const vw::Sampler> m_sampler = vw::SamplerBuilder(m_device).build();
}; 
