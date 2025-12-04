#pragma once

#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"

#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>
#include <cmath>

inline std::shared_ptr<vw::DescriptorSetLayout>
create_sky_pass_descriptor_layout(std::shared_ptr<const vw::Device> device) {
    return vw::DescriptorSetLayoutBuilder(device)
        .with_uniform_buffer(vk::ShaderStageFlagBits::eFragment, 1)
        .build();
}

class SkyPass : public vw::ScreenSpacePass {
  public:
    struct PushConstants {
        glm::vec4 sunDirection; // xyz = direction to sun, w = unused
    };

    SkyPass(std::shared_ptr<const vw::Device> device,
            std::shared_ptr<const vw::Pipeline> pipeline,
            vw::DescriptorSet descriptor_set,
            std::shared_ptr<const vw::ImageView> output_image,
            std::shared_ptr<const vw::ImageView> depth_image,
            const float* sun_angle)
        : ScreenSpacePass(std::move(device), std::move(pipeline),
                          std::move(descriptor_set), std::move(output_image),
                          std::move(depth_image)),
          m_sun_angle(sun_angle) {}

    void execute(vk::CommandBuffer cmd_buffer) const noexcept override {
        float angle_rad = glm::radians(*m_sun_angle);
        PushConstants constants{
            .sunDirection = glm::vec4(std::cos(angle_rad), std::sin(angle_rad),
                                      0.0f, 0.0f)};

        cmd_buffer.pushConstants(m_pipeline->layout().handle(),
                                 vk::ShaderStageFlagBits::eFragment, 0,
                                 sizeof(PushConstants), &constants);

        ScreenSpacePass::execute(cmd_buffer);
    }

  private:
    const float* m_sun_angle; // degrees above horizon (90 = zenith)
};

inline std::shared_ptr<SkyPass> create_sky_pass(
    std::shared_ptr<const vw::Device> device,
    std::shared_ptr<const vw::DescriptorSetLayout> descriptor_set_layout,
    vw::DescriptorSet descriptor_set,
    std::shared_ptr<const vw::ImageView> output_image,
    std::shared_ptr<const vw::ImageView> depth_image,
    const float* sun_angle) {

    auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/fullscreen.spv");
    auto fragment_shader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/post-process/sky.spv");

    std::vector<vk::PushConstantRange> push_constants = {
        vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0,
                              sizeof(SkyPass::PushConstants))};

    auto pipeline = vw::create_screen_space_pipeline(
        device, vertex_shader, fragment_shader, descriptor_set_layout,
        output_image->image()->format(), depth_image->image()->format(), true,
        vk::CompareOp::eEqual, push_constants);

    return std::make_shared<SkyPass>(device, pipeline, descriptor_set,
                                     output_image, depth_image, sun_angle);
}
