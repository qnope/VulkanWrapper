#pragma once

#include "RenderPassInformation.h"
#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include <array>
#include <glm/glm.hpp>
#include <random>

// Maximum number of AO samples supported (must match shader)
constexpr int AO_MAX_SAMPLES = 256;

// UBO structure for AO samples (matches shader layout)
// Each sample contains (xi1, xi2) for cosine-weighted hemisphere sampling
struct AOSamplesUBO {
    // Using vec4 for std140 alignment, only xy are used
    std::array<glm::vec4, AO_MAX_SAMPLES> samples;
};

// Generate random samples for cosine-weighted hemisphere sampling
inline AOSamplesUBO generate_ao_samples() {
    AOSamplesUBO ubo{};

    std::random_device engine;
    std::mt19937 rng(engine());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int i = 0; i < AO_MAX_SAMPLES; ++i) {
        ubo.samples[i] = glm::vec4(dist(rng), dist(rng), 0.0f, 0.0f);
    }

    return ubo;
}

// Create and fill the AO samples buffer
inline vw::Buffer<AOSamplesUBO, true, vw::UniformBufferUsage>
create_ao_samples_buffer(const vw::Allocator &allocator) {
    auto buffer = vw::create_buffer<AOSamplesUBO, true, vw::UniformBufferUsage>(
        allocator, 1);
    auto samples = generate_ao_samples();
    buffer.copy(samples, 0);
    return buffer;
}

inline std::shared_ptr<vw::DescriptorSetLayout>
create_ao_pass_descriptor_layout(std::shared_ptr<const vw::Device> device) {
    return vw::DescriptorSetLayoutBuilder(device)
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Position
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Normal
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Tangent
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Bitangent
        .with_acceleration_structure(vk::ShaderStageFlagBits::eFragment) // TLAS
        .with_uniform_buffer(vk::ShaderStageFlagBits::eFragment,
                             1) // AO samples UBO
        .build();
}

inline vw::DescriptorSet create_ao_pass_descriptor_set(
    vw::DescriptorPool &pool, std::shared_ptr<const vw::Sampler> sampler,
    const GBuffer &gbuffer, vk::AccelerationStructureKHR tlas,
    const vw::Buffer<AOSamplesUBO, true, vw::UniformBufferUsage>
        &ao_samples_buffer) {
    vw::DescriptorAllocator allocator;
    allocator.add_combined_image(0, vw::CombinedImage(gbuffer.position, sampler),
                                 vk::PipelineStageFlagBits2::eFragmentShader,
                                 vk::AccessFlagBits2::eShaderRead);
    allocator.add_combined_image(1, vw::CombinedImage(gbuffer.normal, sampler),
                                 vk::PipelineStageFlagBits2::eFragmentShader,
                                 vk::AccessFlagBits2::eShaderRead);
    allocator.add_combined_image(2,
                                 vw::CombinedImage(gbuffer.tangeant, sampler),
                                 vk::PipelineStageFlagBits2::eFragmentShader,
                                 vk::AccessFlagBits2::eShaderRead);
    allocator.add_combined_image(3,
                                 vw::CombinedImage(gbuffer.biTangeant, sampler),
                                 vk::PipelineStageFlagBits2::eFragmentShader,
                                 vk::AccessFlagBits2::eShaderRead);
    allocator.add_acceleration_structure(
        4, tlas, vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eAccelerationStructureReadKHR);
    allocator.add_uniform_buffer(5, ao_samples_buffer.handle(), 0,
                                 sizeof(AOSamplesUBO),
                                 vk::PipelineStageFlagBits2::eFragmentShader,
                                 vk::AccessFlagBits2::eUniformRead);
    return pool.allocate_set(allocator);
}

class AmbientOcclusionPass : public vw::ScreenSpacePass {
  public:
    struct PushConstants {
        float aoRadius;
        int32_t numSamples;
    };

    AmbientOcclusionPass(std::shared_ptr<const vw::Device> device,
                         std::shared_ptr<const vw::Pipeline> pipeline,
                         vw::DescriptorSet descriptor_set,
                         std::shared_ptr<const vw::ImageView> output_image,
                         float ao_radius, int32_t num_samples)
        : ScreenSpacePass(std::move(device), std::move(pipeline),
                          std::move(descriptor_set), std::move(output_image))
        , m_ao_radius(ao_radius)
        , m_num_samples(num_samples) {}

    void execute(vk::CommandBuffer cmd_buffer) const noexcept override {
        PushConstants constants{
            .aoRadius = m_ao_radius,
            .numSamples = m_num_samples};

        cmd_buffer.pushConstants(m_pipeline->layout().handle(),
                                 vk::ShaderStageFlagBits::eFragment, 0,
                                 sizeof(PushConstants), &constants);

        ScreenSpacePass::execute(cmd_buffer);
    }

    void set_ao_radius(float radius) { m_ao_radius = radius; }
    void set_num_samples(int32_t num_samples) {
        m_num_samples = std::clamp(num_samples, 1, AO_MAX_SAMPLES);
    }

  private:
    float m_ao_radius;
    int32_t m_num_samples;
};

inline std::shared_ptr<AmbientOcclusionPass> create_ao_pass(
    std::shared_ptr<const vw::Device> device,
    std::shared_ptr<const vw::DescriptorSetLayout> descriptor_set_layout,
    vw::DescriptorSet descriptor_set,
    std::shared_ptr<const vw::ImageView> output_image,
    float ao_radius, int32_t num_samples) {

    auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/fullscreen.spv");
    auto fragment_shader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/post-process/ambient_occlusion.spv");

    std::vector<vk::PushConstantRange> push_constants = {
        vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0,
                              sizeof(AmbientOcclusionPass::PushConstants))};

    auto pipeline = vw::create_screen_space_pipeline(
        device, vertex_shader, fragment_shader, descriptor_set_layout,
        output_image->image()->format(), vk::Format::eUndefined, false,
        vk::CompareOp::eAlways, push_constants);

    return std::make_shared<AmbientOcclusionPass>(
        device, pipeline, descriptor_set, output_image,
        ao_radius, num_samples);
}
