#include "RayTracing.h"

#include <filesystem>
#include <VulkanWrapper/Image/CombinedImage.h>
#include <VulkanWrapper/Image/Framebuffer.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Image/Sampler.h>
#include <VulkanWrapper/Memory/Barrier.h>
#include <VulkanWrapper/Pipeline/PipelineLayout.h>
#include <VulkanWrapper/Pipeline/ShaderModule.h>
#include <VulkanWrapper/Utils/exceptions.h>

enum { Color, Position, Normal, Tangeant, BiTangeant, Light, Depth };

RayTracingPass::RayTracingPass(vw::Device &device, vw::Allocator &allocator,
                               const vw::Model::MeshManager &mesh_manager,
                               vw::Width width, vw::Height height)
    : m_width(width)
    , m_height(height)
    , m_device(&device)
    , m_descriptor_set_layout{vw::DescriptorSetLayoutBuilder(device)
                                  .with_combined_image(
                                      vk::ShaderStageFlagBits::eRaygenKHR, 1)
                                  .with_combined_image(
                                      vk::ShaderStageFlagBits::eRaygenKHR, 1)
                                  .with_storage_image(
                                      vk::ShaderStageFlagBits::eRaygenKHR, 1)
                                  .with_acceleration_structure(
                                      vk::ShaderStageFlagBits::eRaygenKHR)
                                  .with_uniform_buffer(
                                      vk::ShaderStageFlagBits::eRaygenKHR, 1)
                                  .build()}
    , m_descriptor_pool{vw::DescriptorPoolBuilder(device,
                                                  m_descriptor_set_layout)
                            .build()}
    , m_sampler{vw::SamplerBuilder(device).build()}
    , m_bottom(
          vw::AccelerationStructure::BottomLevelAccelerationStructureBuilder(
              device, allocator)
              .add_geometries(mesh_manager.meshes())
              .build())
    , m_top(vw::AccelerationStructure::TopLevelAccelerationStructureBuilder(
                device, allocator)
                .add_instance(m_bottom)
                .build()) {
    // Charger les shaders SPIR-V
    auto raygen = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/RayTracing/raygen.rgen.spv");
    auto miss = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/RayTracing/miss.rmiss.spv");
    auto hit = vw::ShaderModule::create_from_spirv_file(
        device, "Shaders/RayTracing/hit.rchit.spv");

    // Créer le layout des descripteurs

    auto pipeline_layout =
        vw::PipelineLayoutBuilder(device)
            .with_descriptor_set_layout(m_descriptor_set_layout)
            .build();

    // Créer le pipeline de ray tracing
    m_pipeline = std::make_shared<vw::rt::RayTracingPipeline>(
        vw::rt::RayTracingPipelineBuilder(device, allocator,
                                          std::move(pipeline_layout))
            .set_ray_generation_shader(raygen)
            .add_miss_shader(miss)
            .add_closest_hit_shader(hit)
            .build());
}

void RayTracingPass::execute(vk::CommandBuffer command_buffer,
                             const vw::Framebuffer &framebuffer,
                             vk::Buffer handle) {
    // Récupérer les vues du depth et du light buffer
    auto light_buffer = framebuffer.image_view(Light); // Light
    auto position_buffer = framebuffer.image_view(Position);
    auto depth_buffer = framebuffer.image_view(Depth); // Depth

    // Préparer le descriptor set
    vw::DescriptorAllocator allocator;
    allocator.add_combined_image(0, vw::CombinedImage(depth_buffer, m_sampler));
    allocator.add_combined_image(1,
                                 vw::CombinedImage(position_buffer, m_sampler));
    allocator.add_storage_image(2, *light_buffer);
    allocator.add_acceleration_structure(3, m_top.handle());
    allocator.add_uniform_buffer(4, handle, 0, sizeof(glm::mat4) * 2);
    m_descriptor_set = m_descriptor_pool.allocate_set(allocator);

    // Bind pipeline et descriptor set
    command_buffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,
                                m_pipeline->handle());
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                                      m_pipeline->layout().handle(), 0,
                                      {m_descriptor_set}, {});

    // const auto shader_binding_table = m_pipeline->get_shader_binding_table();

    vw::execute_image_barrier_general_to_sampled(
        command_buffer, light_buffer->image(),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR);
}
