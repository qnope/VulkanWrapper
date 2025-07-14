#include "VulkanWrapper/Pipeline/RayTracingPipeline.h"

#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

RayTracingPipeline::RayTracingPipeline(vk::UniquePipeline pipeline,
                                       PipelineLayout pipeline_layout,
                                       const Device &device) noexcept
    : ObjectWithUniqueHandle<vk::UniquePipeline>(std::move(pipeline))
    , m_layout(std::move(pipeline_layout))
    , m_shader_group_handle_size(0)
    , m_device(device) {}

const PipelineLayout &RayTracingPipeline::layout() const noexcept {
    return m_layout;
}

vk::DeviceSize
RayTracingPipeline::get_shader_group_handle_size() const noexcept {
    return m_shader_group_handle_size;
}

std::vector<uint8_t>
RayTracingPipeline::get_shader_group_handles() const noexcept {
    const auto handle_size = m_shader_group_handle_size;
    const auto group_count = 2; // raygen + closest_hit + miss

    std::vector<uint8_t> handles(handle_size * group_count);

    // Get shader group handles
    try {
        auto result = m_device.handle().getRayTracingShaderGroupHandlesKHR(
            handle(), 0, group_count, handles.size(), handles.data());
        if (result == vk::Result::eSuccess) {
            return handles;
        }
    } catch (...) {
        // Ignore exceptions
    }
    // Return empty handles if failed
    return std::vector<uint8_t>();
}

RayTracingPipelineBuilder::RayTracingPipelineBuilder(
    const Device &device, PipelineLayout pipelineLayout)
    : m_device(device)
    , m_pipelineLayout(std::move(pipelineLayout)) {}

RayTracingPipelineBuilder &&
RayTracingPipelineBuilder::add_ray_generation_shader(
    std::shared_ptr<const ShaderModule> module) && {
    m_stages.emplace_back()
        .setPName("main")
        .setModule(module->handle())
        .setStage(vk::ShaderStageFlagBits::eRaygenKHR);

    m_groups.emplace_back()
        .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
        .setGeneralShader(m_stages.size() - 1);

    m_all_shaders.push_back(std::move(module));
    return std::move(*this);
}

RayTracingPipelineBuilder &&RayTracingPipelineBuilder::add_closest_hit_shader(
    std::shared_ptr<const ShaderModule> module) && {
    m_stages.emplace_back()
        .setPName("main")
        .setModule(module->handle())
        .setStage(vk::ShaderStageFlagBits::eClosestHitKHR);

    m_groups.emplace_back()
        .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
        .setClosestHitShader(m_stages.size() - 1);

    m_all_shaders.push_back(std::move(module));
    return std::move(*this);
}

RayTracingPipelineBuilder &&RayTracingPipelineBuilder::add_miss_shader(
    std::shared_ptr<const ShaderModule> module) && {
    m_stages.emplace_back()
        .setPName("main")
        .setModule(module->handle())
        .setStage(vk::ShaderStageFlagBits::eMissKHR);

    m_groups.emplace_back()
        .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
        .setGeneralShader(m_stages.size() - 1);

    m_all_shaders.push_back(std::move(module));
    return std::move(*this);
}

RayTracingPipelineBuilder &&RayTracingPipelineBuilder::add_callable_shader(
    std::shared_ptr<const ShaderModule> module) && {
    m_stages.emplace_back()
        .setPName("main")
        .setModule(module->handle())
        .setStage(vk::ShaderStageFlagBits::eCallableKHR);

    m_groups.emplace_back()
        .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
        .setGeneralShader(m_stages.size() - 1);

    m_all_shaders.push_back(std::move(module));
    return std::move(*this);
}

RayTracingPipeline RayTracingPipelineBuilder::build() && {
    const auto info = vk::RayTracingPipelineCreateInfoKHR()
                          .setStages(m_stages)
                          .setGroups(m_groups)
                          .setMaxPipelineRayRecursionDepth(1)
                          .setLayout(m_pipelineLayout.handle());

    auto [result, pipeline] =
        m_device.handle().createRayTracingPipelineKHRUnique(
            vk::DeferredOperationKHR(), vk::PipelineCache(), info);

    if (result != vk::Result::eSuccess) {
        throw RayTracingPipelineCreationException{
            std::source_location::current()};
    }

    // Get shader group handle size
    const auto properties =
        m_device.physical_device()
            .getProperties2<
                vk::PhysicalDeviceProperties2,
                vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    const auto rayTracingProperties =
        properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    const auto shaderGroupHandleSize =
        rayTracingProperties.shaderGroupHandleSize;

    RayTracingPipeline rayTracingPipeline(
        std::move(pipeline), std::move(m_pipelineLayout), m_device);
    rayTracingPipeline.m_shader_group_handle_size = shaderGroupHandleSize;

    return rayTracingPipeline;
}

} // namespace vw
