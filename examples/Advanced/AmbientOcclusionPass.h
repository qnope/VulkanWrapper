#pragma once

#include "RenderPassInformation.h"
#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/Sampler.h"

#include <glm/glm.hpp>

inline std::shared_ptr<vw::DescriptorSetLayout>
create_ao_pass_descriptor_layout(std::shared_ptr<const vw::Device> device) {
    return vw::DescriptorSetLayoutBuilder(device)
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Depth
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Normal
        .with_acceleration_structure(vk::ShaderStageFlagBits::eFragment) // TLAS
        .build();
}

inline vw::DescriptorSet create_ao_pass_descriptor_set(
    vw::DescriptorPool &pool,
    std::shared_ptr<const vw::Sampler> sampler,
    const GBuffer &gbuffer,
    vk::AccelerationStructureKHR tlas) {
    vw::DescriptorAllocator allocator;
    allocator.add_combined_image(
        0, vw::CombinedImage(gbuffer.depth, sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    allocator.add_combined_image(
        1, vw::CombinedImage(gbuffer.normal, sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    allocator.add_acceleration_structure(
        2, tlas,
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eAccelerationStructureReadKHR);
    return pool.allocate_set(allocator);
}

class AmbientOcclusionPass : public vw::ScreenSpacePass {
  public:
    struct PushConstants {
        glm::mat4 inverseViewProj;
        float aoRadius;      // Maximum distance for occlusion check
        float aoIntensity;   // How strong the AO effect is
        int aoSamples;       // Number of ray samples per pixel
        uint32_t frameIndex; // For temporal noise variation
    };

    AmbientOcclusionPass(std::shared_ptr<const vw::Device> device,
                         std::shared_ptr<const vw::Pipeline> pipeline,
                         vw::DescriptorSet descriptor_set,
                         std::shared_ptr<const vw::ImageView> output_image,
                         const UBOData* ubo_data,
                         float ao_radius = 30.0f,
                         float ao_intensity = 4.0f,
                         int ao_samples = 32)
        : ScreenSpacePass(std::move(device), std::move(pipeline),
                          std::move(descriptor_set), std::move(output_image)),
          m_ubo_data(ubo_data),
          m_ao_radius(ao_radius),
          m_ao_intensity(ao_intensity),
          m_ao_samples(ao_samples) {}

    void execute(vk::CommandBuffer cmd_buffer) const noexcept override {
        PushConstants constants{
            .inverseViewProj = m_ubo_data->inverseViewProj,
            .aoRadius = m_ao_radius,
            .aoIntensity = m_ao_intensity,
            .aoSamples = m_ao_samples,
            .frameIndex = m_frame_index++};

        cmd_buffer.pushConstants(m_pipeline->layout().handle(),
                                 vk::ShaderStageFlagBits::eFragment, 0,
                                 sizeof(PushConstants), &constants);

        ScreenSpacePass::execute(cmd_buffer);
    }

    void set_ao_radius(float radius) { m_ao_radius = radius; }
    void set_ao_intensity(float intensity) { m_ao_intensity = intensity; }
    void set_ao_samples(int samples) { m_ao_samples = samples; }

  private:
    const UBOData* m_ubo_data;
    float m_ao_radius;
    float m_ao_intensity;
    int m_ao_samples;
    mutable uint32_t m_frame_index = 0;
};

inline std::shared_ptr<AmbientOcclusionPass> create_ao_pass(
    std::shared_ptr<const vw::Device> device,
    std::shared_ptr<const vw::DescriptorSetLayout> descriptor_set_layout,
    vw::DescriptorSet descriptor_set,
    std::shared_ptr<const vw::ImageView> output_image,
    const UBOData* ubo_data,
    float ao_radius = 30.0f,
    float ao_intensity = 4.0f,
    int ao_samples = 32) {

    auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/fullscreen.spv");
    auto fragment_shader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/post-process/ao.spv");

    std::vector<vk::PushConstantRange> push_constants = {
        vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0,
                              sizeof(AmbientOcclusionPass::PushConstants))};

    auto pipeline = vw::create_screen_space_pipeline(
        device, vertex_shader, fragment_shader, descriptor_set_layout,
        output_image->image()->format(), vk::Format::eUndefined, false,
        vk::CompareOp::eAlways, push_constants);

    return std::make_shared<AmbientOcclusionPass>(
        device, pipeline, descriptor_set, output_image, ubo_data,
        ao_radius, ao_intensity, ao_samples);
}
