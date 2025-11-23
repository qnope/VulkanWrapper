#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include <VulkanWrapper/Descriptors/DescriptorAllocator.h>
#include <VulkanWrapper/Descriptors/DescriptorPool.h>
#include <VulkanWrapper/Descriptors/DescriptorSetLayout.h>
#include <VulkanWrapper/fwd.h>
#include <VulkanWrapper/Image/Image.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/RayTracing/RayTracingPipeline.h>

class RayTracingPass {
  public:
    RayTracingPass(vw::Device &device, vw::Allocator &allocator,
                   const vw::Model::MeshManager &mesh_manager, vw::Width width,
                   vw::Height height);
    void execute(vk::CommandBuffer command_buffer,
                 const vw::Framebuffer &framebuffer, vk::Buffer handle);

  private:
    vw::Width m_width;
    vw::Height m_height;
    const vw::Device *m_device;

    std::shared_ptr<vw::rt::RayTracingPipeline> m_pipeline;
    std::shared_ptr<vw::DescriptorSetLayout> m_descriptor_set_layout;
    vw::DescriptorPool m_descriptor_pool;
    vk::DescriptorSet m_descriptor_set;

    std::shared_ptr<const vw::Sampler> m_sampler;
};
