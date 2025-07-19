#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

using ShaderBindingTableBuffer =
    Buffer<std::byte, true,
           VkBufferUsageFlags(vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                              vk::BufferUsageFlagBits::eShaderDeviceAddress)>;

struct ShaderBindingTable {
    vk::StridedDeviceAddressRegionKHR generation_region;
    vk::StridedDeviceAddressRegionKHR closest_hit_region;
    vk::StridedDeviceAddressRegionKHR miss_region;
};

using RayTracingPipelineCreationException =
    TaggedException<struct RayTracingPipelineCreationTag>;

class RayTracingPipeline : public ObjectWithUniqueHandle<vk::UniquePipeline> {
    friend class RayTracingPipelineBuilder;

  public:
    RayTracingPipeline(vk::UniquePipeline pipeline,
                       PipelineLayout pipeline_layout,
                       ShaderBindingTableBuffer shader_binding_table_buffer,
                       ShaderBindingTable shader_binding_table) noexcept;

    [[nodiscard]] const PipelineLayout &layout() const noexcept;

    [[nodiscard]] ShaderBindingTable get_shader_binding_table() const noexcept;

  private:
    PipelineLayout m_layout;
    ShaderBindingTableBuffer m_shader_binding_table_buffer;
    ShaderBindingTable m_shader_binding_table;
};

class RayTracingPipelineBuilder {
  public:
    RayTracingPipelineBuilder(const Device &device, const Allocator &allocator,
                              PipelineLayout pipelineLayout);

    RayTracingPipelineBuilder &&
    set_ray_generation_shader(std::shared_ptr<const ShaderModule> module) &&;

    RayTracingPipelineBuilder &&
    add_closest_hit_shader(std::shared_ptr<const ShaderModule> module) &&;

    RayTracingPipelineBuilder &&
    add_miss_shader(std::shared_ptr<const ShaderModule> module) &&;

    RayTracingPipeline build() &&;

  private:
    std::vector<vk::PipelineShaderStageCreateInfo> create_stages() const;
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> create_groups() const;

    std::tuple<ShaderBindingTable, ShaderBindingTableBuffer>
    create_shader_binding_table(vk::Pipeline pipeline) const;

  private:
    const Device &m_device;
    const Allocator &m_allocator;
    PipelineLayout m_pipelineLayout;

    std::shared_ptr<const ShaderModule> m_ray_generation_shader;
    std::vector<std::shared_ptr<const ShaderModule>> m_closest_hit_shaders;
    std::vector<std::shared_ptr<const ShaderModule>> m_miss_shaders;
};

} // namespace vw
