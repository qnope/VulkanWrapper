#pragma once

#include "RenderPassInformation.h"
#include "ZPass.h"
#include "ColorPass.h"
#include "SkyPass.h"
#include "SunLightPass.h"

#include <VulkanWrapper/Descriptors/DescriptorPool.h>
#include <VulkanWrapper/Descriptors/DescriptorSetLayout.h>
#include <VulkanWrapper/Image/Sampler.h>
#include <VulkanWrapper/Memory/Buffer.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/Pipeline/MeshRenderer.h>
#include <VulkanWrapper/RenderPass/Rendering.h>
#include <VulkanWrapper/Vulkan/Device.h>
#include <VulkanWrapper/Vulkan/Swapchain.h>

#include <memory>
#include <vector>

class DeferredRenderingManager {
  public:
    struct Config {
        std::vector<vk::Format> gbuffer_color_formats = {
            vk::Format::eR8G8B8A8Unorm,      // color
            vk::Format::eR32G32B32A32Sfloat, // position
            vk::Format::eR32G32B32A32Sfloat, // normal
            vk::Format::eR32G32B32A32Sfloat, // tangent
            vk::Format::eR32G32B32A32Sfloat, // bitangent
            vk::Format::eR32G32B32A32Sfloat  // light
        };
        vk::Format depth_format = vk::Format::eD32Sfloat;
    };

    DeferredRenderingManager(
        std::shared_ptr<vw::Device> device,
        std::shared_ptr<vw::Allocator> allocator,
        const vw::Swapchain &swapchain,
        vw::Model::MeshManager &mesh_manager,
        const vw::Buffer<UBOData, true, vw::UniformBufferUsage> &uniform_buffer,
        Config config);

    DeferredRenderingManager(
        std::shared_ptr<vw::Device> device,
        std::shared_ptr<vw::Allocator> allocator,
        const vw::Swapchain &swapchain,
        vw::Model::MeshManager &mesh_manager,
        const vw::Buffer<UBOData, true, vw::UniformBufferUsage> &uniform_buffer)
        : DeferredRenderingManager(std::move(device), std::move(allocator),
                                   swapchain, mesh_manager, uniform_buffer,
                                   Config{}) {}

    const std::vector<vw::Rendering> &renderings() const { return m_renderings; }

    const std::vector<GBuffer> &gbuffers() const { return m_gbuffers; }

    std::shared_ptr<vw::DescriptorSetLayout> uniform_descriptor_layout() const {
        return m_uniform_descriptor_layout;
    }

    vw::DescriptorSet uniform_descriptor_set() const {
        return *m_uniform_descriptor_set;
    }

  private:
    void create_gbuffers(const vw::Swapchain &swapchain);
    void create_uniform_descriptors(
        const vw::Buffer<UBOData, true, vw::UniformBufferUsage> &uniform_buffer);
    void create_zpass_resources();
    void create_color_pass_resources(vw::Model::MeshManager &mesh_manager);
    void create_sun_light_pass_resources();
    void create_sky_pass_resources();
    void create_renderings(vw::Model::MeshManager &mesh_manager);

    std::shared_ptr<vw::Device> m_device;
    std::shared_ptr<vw::Allocator> m_allocator;
    Config m_config;

    // GBuffers (one per swapchain image)
    std::vector<GBuffer> m_gbuffers;

    // Shared uniform buffer descriptor
    std::shared_ptr<vw::DescriptorSetLayout> m_uniform_descriptor_layout;
    std::optional<vw::DescriptorPool> m_uniform_descriptor_pool;
    std::optional<vw::DescriptorSet> m_uniform_descriptor_set;

    // ZPass resources
    std::shared_ptr<const vw::Pipeline> m_zpass_pipeline;

    // Color pass resources
    std::shared_ptr<vw::MeshRenderer> m_mesh_renderer;

    // Sun light pass resources
    std::shared_ptr<vw::DescriptorSetLayout> m_sunlight_descriptor_layout;
    std::optional<vw::DescriptorPool> m_sunlight_descriptor_pool;
    std::vector<vw::DescriptorSet> m_sunlight_descriptor_sets;

    // Sampler for post-process passes
    std::shared_ptr<const vw::Sampler> m_sampler;

    // Final renderings (one per swapchain image)
    std::vector<vw::Rendering> m_renderings;
};
