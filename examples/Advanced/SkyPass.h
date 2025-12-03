#pragma once

#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"

inline std::shared_ptr<vw::DescriptorSetLayout>
create_sky_pass_descriptor_layout(std::shared_ptr<const vw::Device> device) {
    return vw::DescriptorSetLayoutBuilder(device)
        .with_uniform_buffer(vk::ShaderStageFlagBits::eFragment, 1)
        .build();
}

inline std::shared_ptr<vw::ScreenSpacePass> create_sky_pass(
    std::shared_ptr<const vw::Device> device,
    std::shared_ptr<const vw::DescriptorSetLayout> descriptor_set_layout,
    vw::DescriptorSet descriptor_set,
    std::shared_ptr<const vw::ImageView> output_image,
    std::shared_ptr<const vw::ImageView> depth_image) {

    auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/fullscreen.spv");
    auto fragment_shader = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/post-process/sky.spv");

    auto pipeline = vw::create_screen_space_pipeline(
        device, vertex_shader, fragment_shader, descriptor_set_layout,
        output_image->image()->format(), depth_image->image()->format(), true,
        vk::CompareOp::eEqual);

    return std::make_shared<vw::ScreenSpacePass>(
        device, pipeline, descriptor_set, output_image, depth_image);
}
