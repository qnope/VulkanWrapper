#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

using RayTracingPipelineCreationException =
    TaggedException<struct RayTracingPipelineCreationTag>;

class RayTracingPipeline : public ObjectWithUniqueHandle<vk::UniquePipeline> {
    friend class RayTracingPipelineBuilder;

  public:
    RayTracingPipeline(vk::UniquePipeline pipeline,
                       PipelineLayout pipeline_layout,
                       const Device &device) noexcept;

    // Move-only class
    RayTracingPipeline(RayTracingPipeline &&other) noexcept = default;
    RayTracingPipeline &operator=(RayTracingPipeline &&other) noexcept = delete;
    RayTracingPipeline(const RayTracingPipeline &) = delete;
    RayTracingPipeline &operator=(const RayTracingPipeline &) = delete;

    [[nodiscard]] const PipelineLayout &layout() const noexcept;
    [[nodiscard]] vk::DeviceSize get_shader_group_handle_size() const noexcept;
    [[nodiscard]] std::vector<uint8_t>
    get_shader_group_handles() const noexcept;

  private:
    PipelineLayout m_layout;
    vk::DeviceSize m_shader_group_handle_size;
    const Device &m_device;
};

class RayTracingPipelineBuilder {
  public:
    RayTracingPipelineBuilder(const Device &device,
                              PipelineLayout pipelineLayout);

    RayTracingPipelineBuilder &&
    add_ray_generation_shader(std::shared_ptr<const ShaderModule> module) &&;

    RayTracingPipelineBuilder &&
    add_closest_hit_shader(std::shared_ptr<const ShaderModule> module) &&;

    RayTracingPipelineBuilder &&
    add_miss_shader(std::shared_ptr<const ShaderModule> module) &&;

    RayTracingPipelineBuilder &&
    add_callable_shader(std::shared_ptr<const ShaderModule> module) &&;

    RayTracingPipeline build() &&;

  private:
    const Device &m_device;
    PipelineLayout m_pipelineLayout;

    std::vector<std::shared_ptr<const ShaderModule>> m_all_shaders;

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_groups;
    std::vector<vk::PipelineShaderStageCreateInfo> m_stages;
};

} // namespace vw
