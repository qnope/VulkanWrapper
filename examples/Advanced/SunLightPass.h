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
#include <glm/trigonometric.hpp>
#include <cmath>

inline std::shared_ptr<vw::DescriptorSetLayout>
create_sun_light_pass_descriptor_layout(std::shared_ptr<const vw::Device> device) {
    return vw::DescriptorSetLayoutBuilder(device)
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Color
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Depth
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Normal
        .with_acceleration_structure(vk::ShaderStageFlagBits::eFragment) // TLAS
        .build();
}

inline vw::DescriptorSet create_sun_light_pass_descriptor_set(
    vw::DescriptorPool &pool,
    std::shared_ptr<const vw::Sampler> sampler,
    const GBuffer &gbuffer,
    vk::AccelerationStructureKHR tlas) {
    vw::DescriptorAllocator allocator;
    allocator.add_combined_image(
        0, vw::CombinedImage(gbuffer.color, sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    allocator.add_combined_image(
        1, vw::CombinedImage(gbuffer.depth, sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    allocator.add_combined_image(
        2, vw::CombinedImage(gbuffer.normal, sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    allocator.add_acceleration_structure(
        3, tlas,
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eAccelerationStructureReadKHR);
    return pool.allocate_set(allocator);
}

class SunLightPass : public vw::ScreenSpacePass {
  public:
    struct PushConstants {
        glm::vec4 sunDirection;
        glm::vec4 sunColor;
        glm::mat4 inverseViewProj;
    };

    SunLightPass(std::shared_ptr<const vw::Device> device,
                 std::shared_ptr<const vw::Pipeline> pipeline,
                 vw::DescriptorSet descriptor_set,
                 std::shared_ptr<const vw::ImageView> output_image,
                 const float* sun_angle,
                 const UBOData* ubo_data)
        : ScreenSpacePass(std::move(device), std::move(pipeline),
                          std::move(descriptor_set), std::move(output_image)),
          m_sun_angle(sun_angle),
          m_ubo_data(ubo_data) {}

    void execute(vk::CommandBuffer cmd_buffer) const noexcept override {
        // Compute sun direction from angle (direction FROM sun TO surface)
        // This is the negative of the direction TO the sun
        float angle_rad = glm::radians(*m_sun_angle);
        PushConstants constants{
            .sunDirection = glm::vec4(-std::cos(angle_rad), -std::sin(angle_rad),
                                      0.0f, 0.0f),
            .sunColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .inverseViewProj = m_ubo_data->inverseViewProj};

        cmd_buffer.pushConstants(m_pipeline->layout().handle(),
                                 vk::ShaderStageFlagBits::eFragment, 0,
                                 sizeof(PushConstants), &constants);

        ScreenSpacePass::execute(cmd_buffer);
    }

  private:
    const float* m_sun_angle; // degrees above horizon (90 = zenith)
    const UBOData* m_ubo_data;
};

inline std::shared_ptr<SunLightPass> create_sun_light_pass(
    std::shared_ptr<const vw::Device> device,
    std::shared_ptr<const vw::DescriptorSetLayout> descriptor_set_layout,
    vw::DescriptorSet descriptor_set,
    std::shared_ptr<const vw::ImageView> output_image,
    const float* sun_angle,
    const UBOData* ubo_data) {

    auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/fullscreen.spv");
    auto fragment_shader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/post-process/sun_light.spv");

    std::vector<vk::PushConstantRange> push_constants = {
        vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0,
                              sizeof(SunLightPass::PushConstants))};

    auto pipeline = vw::create_screen_space_pipeline(
        device, vertex_shader, fragment_shader, descriptor_set_layout,
        output_image->image()->format(), vk::Format::eUndefined, false,
        vk::CompareOp::eAlways, push_constants);

    return std::make_shared<SunLightPass>(device, pipeline, descriptor_set,
                                          output_image, sun_angle, ubo_data);
}
