#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw::rt {

constexpr uint64_t ShaderBindingTableHandleSizeAlignment = 64;
using ShaderBindingTableHandle = std::vector<std::byte>;

class RayTracingPipeline : public ObjectWithUniqueHandle<vk::UniquePipeline> {
    friend class RayTracingPipelineBuilder;

    RayTracingPipeline(std::shared_ptr<const Device> device,
                       vk::UniquePipeline pipeline,
                       PipelineLayout pipeline_layout,
                       uint32_t number_miss_shader,
                       uint32_t number_close_hit_shader);

  public:
    [[nodiscard]] const PipelineLayout &layout() const noexcept;

    [[nodiscard]] ShaderBindingTableHandle ray_generation_handle() const;
    [[nodiscard]] std::span<const ShaderBindingTableHandle>
    miss_handles() const;
    [[nodiscard]] std::span<const ShaderBindingTableHandle>
    closest_hit_handles() const;

    [[nodiscard]] vk::PipelineLayout handle_layout() const;

  private:
    PipelineLayout m_layout;
    uint32_t m_number_miss_shader;
    uint32_t m_number_close_hit_shader;
    std::vector<ShaderBindingTableHandle> m_handles;
};

class RayTracingPipelineBuilder {
  public:
    RayTracingPipelineBuilder(std::shared_ptr<const Device> device,
                              std::shared_ptr<const Allocator> allocator,
                              PipelineLayout pipelineLayout);

    RayTracingPipelineBuilder &
    set_ray_generation_shader(std::shared_ptr<const ShaderModule> module);

    RayTracingPipelineBuilder &
    add_closest_hit_shader(std::shared_ptr<const ShaderModule> module);

    RayTracingPipelineBuilder &
    add_miss_shader(std::shared_ptr<const ShaderModule> module);

    RayTracingPipeline build();

  private:
    [[nodiscard]] std::vector<vk::PipelineShaderStageCreateInfo>
    create_stages() const;
    [[nodiscard]] std::vector<vk::RayTracingShaderGroupCreateInfoKHR>
    create_groups() const;

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Allocator> m_allocator;
    PipelineLayout m_pipelineLayout;

    std::shared_ptr<const ShaderModule> m_ray_generation_shader;
    std::vector<std::shared_ptr<const ShaderModule>> m_miss_shaders;
    std::vector<std::shared_ptr<const ShaderModule>> m_closest_hit_shaders;
};

} // namespace vw::rt
