#include "VulkanWrapper/RayTracing/RayTracingPipeline.h"

#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw::rt {

RayTracingPipeline::RayTracingPipeline(
    const Device &device, vk::UniquePipeline pipeline,
    PipelineLayout pipeline_layout, uint32_t number_miss_shader,
    uint32_t number_close_hit_shader) noexcept
    : ObjectWithUniqueHandle<vk::UniquePipeline>(std::move(pipeline))
    , m_layout(std::move(pipeline_layout))
    , m_number_miss_shader{number_miss_shader}
    , m_number_close_hit_shader{number_close_hit_shader} {

    auto rayTracingPipelineProperties =
        device.physical_device()
            .getProperties2<vk::PhysicalDeviceProperties2,
                            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>()
            .get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    const uint32_t handleSize =
        rayTracingPipelineProperties.shaderGroupHandleSize;

    const uint32_t groupCount =
        1 + number_miss_shader + number_close_hit_shader;

    std::vector<std::byte> shaderHandleStorage =
        device.handle()
            .getRayTracingShaderGroupHandlesKHR<std::byte>(
                *pipeline, 0, groupCount, handleSize * groupCount)
            .value;

    assert(shaderHandleStorage.size() % handleSize == 0);
    for (int i = 0; i < shaderHandleStorage.size(); i += handleSize)
        m_handles.emplace_back(std::span(&shaderHandleStorage[i], handleSize));
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
                     m_number_miss_shader);
}

RayTracingPipelineBuilder::RayTracingPipelineBuilder(
    const Device &device, const Allocator &allocator,
    PipelineLayout pipelineLayout)
    : m_device(device)
    , m_allocator{allocator}
    , m_pipelineLayout(std::move(pipelineLayout)) {}

RayTracingPipelineBuilder &&
RayTracingPipelineBuilder::set_ray_generation_shader(
    std::shared_ptr<const ShaderModule> module) && {
    m_ray_generation_shader = std::move(module);
    return std::move(*this);
}

RayTracingPipelineBuilder &&RayTracingPipelineBuilder::add_closest_hit_shader(
    std::shared_ptr<const ShaderModule> module) && {
    m_closest_hit_shaders.push_back(std::move(module));
    return std::move(*this);
}

RayTracingPipelineBuilder &&RayTracingPipelineBuilder::add_miss_shader(
    std::shared_ptr<const ShaderModule> module) && {
    m_miss_shaders.push_back(std::move(module));
    return std::move(*this);
}

RayTracingPipeline RayTracingPipelineBuilder::build() && {
    const auto stages = create_stages();
    const auto groups = create_groups();

    const auto info = vk::RayTracingPipelineCreateInfoKHR()
                          .setStages(stages)
                          .setGroups(groups)
                          .setMaxPipelineRayRecursionDepth(1)
                          .setLayout(m_pipelineLayout.handle());

    auto [result, pipeline] =
        m_device.handle().createRayTracingPipelineKHRUnique(
            vk::DeferredOperationKHR(), vk::PipelineCache(), info);

    if (result != vk::Result::eSuccess) {
        throw RayTracingPipelineCreationException{
            std::source_location::current()};
    }

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
