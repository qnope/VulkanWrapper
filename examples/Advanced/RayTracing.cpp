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
    , m_sampler{vw::SamplerBuilder(device).build()} {}

void RayTracingPass::execute(vk::CommandBuffer command_buffer,
                             const vw::Framebuffer &framebuffer,
                             vk::Buffer handle) {}
