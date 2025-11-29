#pragma once

#include "RenderPassInformation.h"
#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/Sampler.h"

inline std::shared_ptr<vw::DescriptorSetLayout>
create_sun_light_pass_descriptor_layout(const vw::Device &device) {
    return vw::DescriptorSetLayoutBuilder(device)
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Color
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Position
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1) // Normal
        .build();
}

inline vw::DescriptorSet create_sun_light_pass_descriptor_set(
    vw::DescriptorPool &pool,
    std::shared_ptr<const vw::Sampler> sampler,
    const GBuffer &gbuffer) {
    vw::DescriptorAllocator allocator;
    allocator.add_combined_image(
        0, vw::CombinedImage(gbuffer.color, sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    allocator.add_combined_image(
        1, vw::CombinedImage(gbuffer.position, sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    allocator.add_combined_image(
        2, vw::CombinedImage(gbuffer.normal, sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    return pool.allocate_set(allocator);
}

class SunLightPass : public vw::ScreenSpacePass {
  public:
    struct PushConstants {
        glm::vec4 sunDirection;
        glm::vec4 sunColor;
    };

    SunLightPass(const vw::Device &device,
                 std::shared_ptr<const vw::Pipeline> pipeline,
                 vw::DescriptorSet descriptor_set,
                 std::shared_ptr<const vw::ImageView> output_image)
        : ScreenSpacePass(device, std::move(pipeline),
                          std::move(descriptor_set), std::move(output_image)) {}

    void execute(vk::CommandBuffer cmd_buffer) const noexcept override {
        PushConstants constants{
            .sunDirection = glm::vec4(0.0f, -1.0f, -0.5f, 0.0f),
            .sunColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)};

        cmd_buffer.pushConstants(m_pipeline->layout().handle(),
                                 vk::ShaderStageFlagBits::eFragment, 0,
                                 sizeof(PushConstants), &constants);

        ScreenSpacePass::execute(cmd_buffer);
    }
};

inline std::shared_ptr<vw::ScreenSpacePass> create_sun_light_pass(
    const vw::Device &device,
    std::shared_ptr<const vw::DescriptorSetLayout> descriptor_set_layout,
    vw::DescriptorSet descriptor_set,
    std::shared_ptr<const vw::ImageView> output_image) {

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
                                          output_image);
}
