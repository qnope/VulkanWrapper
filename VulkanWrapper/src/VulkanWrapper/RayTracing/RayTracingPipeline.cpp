#include "VulkanWrapper/RayTracing/RayTracingPipeline.h"

#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw::rt {

RayTracingPipeline::RayTracingPipeline(std::shared_ptr<const Device> device,
                                       vk::UniquePipeline pipeline,
                                       PipelineLayout pipeline_layout,
                                       uint32_t number_miss_shader,
                                       uint32_t number_close_hit_shader)
    : ObjectWithUniqueHandle<vk::UniquePipeline>(std::move(pipeline))
    , m_layout(std::move(pipeline_layout))
    , m_number_miss_shader{number_miss_shader}
    , m_number_close_hit_shader{number_close_hit_shader} {

    auto rayTracingPipelineProperties =
        device->physical_device()
            .getProperties2<vk::PhysicalDeviceProperties2,
                            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>()
            .get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    const uint32_t handleSize =
        rayTracingPipelineProperties.shaderGroupHandleSize;

    const uint32_t groupCount =
        1 + number_miss_shader + number_close_hit_shader;

    std::vector<std::byte> shaderHandleStorage =
        device->handle()
            .getRayTracingShaderGroupHandlesKHR<std::byte>(
                handle(), 0, groupCount, handleSize * groupCount)
            .value;

    if (shaderHandleStorage.size() % handleSize != 0) {
        throw LogicException::invalid_state(
            "Shader handle storage size is not aligned to handle size");
    }
    for (int i = 0; i < shaderHandleStorage.size(); i += handleSize)
        m_handles.emplace_back(&shaderHandleStorage[i],
                               &shaderHandleStorage[i] + handleSize);
}

const PipelineLayout &RayTracingPipeline::layout() const noexcept {
    return m_layout;
}

ShaderBindingTableHandle RayTracingPipeline::ray_generation_handle() const {
    return m_handles.front();
}

std::span<const ShaderBindingTableHandle>
RayTracingPipeline::miss_handles() const {
    return std::span(m_handles.data() + 1, m_number_miss_shader);
}

std::span<const ShaderBindingTableHandle>
RayTracingPipeline::closest_hit_handles() const {
    return std::span(m_handles.data() + 1 + m_number_miss_shader,
                     m_number_close_hit_shader);
}

vk::PipelineLayout RayTracingPipeline::handle_layout() const {
    return m_layout.handle();
}

RayTracingPipelineBuilder::RayTracingPipelineBuilder(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const Allocator> allocator, PipelineLayout pipelineLayout)
    : m_device(std::move(device))
    , m_allocator{std::move(allocator)}
    , m_pipelineLayout(std::move(pipelineLayout)) {}

RayTracingPipelineBuilder &RayTracingPipelineBuilder::set_ray_generation_shader(
    std::shared_ptr<const ShaderModule> module) {
    m_ray_generation_shader = std::move(module);
    return *this;
}

RayTracingPipelineBuilder &RayTracingPipelineBuilder::add_closest_hit_shader(
    std::shared_ptr<const ShaderModule> module) {
    m_closest_hit_shaders.push_back(std::move(module));
    return *this;
}

RayTracingPipelineBuilder &RayTracingPipelineBuilder::add_miss_shader(
    std::shared_ptr<const ShaderModule> module) {
    m_miss_shaders.push_back(std::move(module));
    return *this;
}

RayTracingPipeline RayTracingPipelineBuilder::build() {
    const auto stages = create_stages();
    const auto groups = create_groups();

    const auto info = vk::RayTracingPipelineCreateInfoKHR()
                          .setStages(stages)
                          .setGroups(groups)
                          .setMaxPipelineRayRecursionDepth(1)
                          .setLayout(m_pipelineLayout.handle());

    auto pipeline =
        check_vk(m_device->handle().createRayTracingPipelineKHRUnique(
                     vk::DeferredOperationKHR(), vk::PipelineCache(), info),
                 "Failed to create ray tracing pipeline");

    RayTracingPipeline rayTracingPipeline(
        m_device, std::move(pipeline), std::move(m_pipelineLayout),
        m_miss_shaders.size(), m_closest_hit_shaders.size());

    return rayTracingPipeline;
}

std::vector<vk::PipelineShaderStageCreateInfo>
RayTracingPipelineBuilder::create_stages() const {
    std::vector<vk::PipelineShaderStageCreateInfo> stages;

    stages.emplace_back()
        .setPName("main")
        .setModule(m_ray_generation_shader->handle())
        .setStage(vk::ShaderStageFlagBits::eRaygenKHR);

    for (const auto &module : m_miss_shaders) {
        stages.emplace_back()
            .setPName("main")
            .setModule(module->handle())
            .setStage(vk::ShaderStageFlagBits::eMissKHR);
    }

    for (const auto &module : m_closest_hit_shaders) {
        stages.emplace_back()
            .setPName("main")
            .setModule(module->handle())
            .setStage(vk::ShaderStageFlagBits::eClosestHitKHR);
    }

    return stages;
}

std::vector<vk::RayTracingShaderGroupCreateInfoKHR>
RayTracingPipelineBuilder::create_groups() const {
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups;

    groups.emplace_back()
        .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
        .setGeneralShader(0);

    for (const auto &module : m_miss_shaders) {
        groups.emplace_back()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(groups.size() - 1);
    }

    for (const auto &module : m_closest_hit_shaders) {
        groups.emplace_back()
            .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
            .setClosestHitShader(groups.size() - 1);
    }

    return groups;
}

} // namespace vw::rt
