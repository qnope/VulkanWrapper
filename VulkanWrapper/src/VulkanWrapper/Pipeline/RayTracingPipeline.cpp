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

vk::DeviceSize RayTracingPipeline::get_shader_group_handle_size() const noexcept {
    return m_shader_group_handle_size;
}

std::vector<uint8_t> RayTracingPipeline::get_shader_group_handles() const noexcept {
    const auto handle_size = m_shader_group_handle_size;
    const auto group_count = 3; // raygen + closest_hit + miss
    
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

RayTracingPipelineBuilder::RayTracingPipelineBuilder(const Device &device,
                                                   PipelineLayout pipelineLayout)
    : m_device(device)
    , m_pipelineLayout(std::move(pipelineLayout)) {}

RayTracingPipelineBuilder &&RayTracingPipelineBuilder::add_ray_generation_shader(
    std::shared_ptr<const ShaderModule> module) && {
    m_rayGenShaders.push_back(std::move(module));
    return std::move(*this);
}

RayTracingPipelineBuilder &&RayTracingPipelineBuilder::add_closest_hit_shader(
    std::shared_ptr<const ShaderModule> module) && {
    m_closestHitShaders.push_back(std::move(module));
    return std::move(*this);
}

RayTracingPipelineBuilder &&RayTracingPipelineBuilder::add_miss_shader(
    std::shared_ptr<const ShaderModule> module) && {
    m_missShaders.push_back(std::move(module));
    return std::move(*this);
}

RayTracingPipelineBuilder &&RayTracingPipelineBuilder::add_callable_shader(
    std::shared_ptr<const ShaderModule> module) && {
    m_callableShaders.push_back(std::move(module));
    return std::move(*this);
}

std::vector<vk::PipelineShaderStageCreateInfo>
RayTracingPipelineBuilder::createShaderStageInfos() const {
    std::vector<vk::PipelineShaderStageCreateInfo> stages;
    
    // Add ray generation shaders
    for (const auto &shader : m_rayGenShaders) {
        stages.emplace_back()
            .setStage(vk::ShaderStageFlagBits::eRaygenKHR)
            .setModule(shader->handle())
            .setPName("main");
    }
    
    // Add closest hit shaders
    for (const auto &shader : m_closestHitShaders) {
        stages.emplace_back()
            .setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
            .setModule(shader->handle())
            .setPName("main");
    }
    
    // Add miss shaders
    for (const auto &shader : m_missShaders) {
        stages.emplace_back()
            .setStage(vk::ShaderStageFlagBits::eMissKHR)
            .setModule(shader->handle())
            .setPName("main");
    }
    
    // Add callable shaders
    for (const auto &shader : m_callableShaders) {
        stages.emplace_back()
            .setStage(vk::ShaderStageFlagBits::eCallableKHR)
            .setModule(shader->handle())
            .setPName("main");
    }
    
    return stages;
}

std::vector<vk::RayTracingShaderGroupCreateInfoKHR>
RayTracingPipelineBuilder::createShaderGroupInfos() const {
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups;
    
    // Create ray generation group
    if (!m_rayGenShaders.empty()) {
        groups.emplace_back()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(0) // First shader in the pipeline
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR);
    }
    
    // Create closest hit group
    if (!m_closestHitShaders.empty()) {
        const uint32_t closestHitIndex = m_rayGenShaders.size();
        groups.emplace_back()
            .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
            .setGeneralShader(VK_SHADER_UNUSED_KHR)
            .setClosestHitShader(closestHitIndex)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR);
    }
    
    // Create miss group
    if (!m_missShaders.empty()) {
        const uint32_t missIndex = m_rayGenShaders.size() + m_closestHitShaders.size();
        groups.emplace_back()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(missIndex)
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR);
    }
    
    return groups;
}

RayTracingPipeline RayTracingPipelineBuilder::build() && {
    const auto shaderStages = createShaderStageInfos();
    const auto shaderGroups = createShaderGroupInfos();
    
    const auto info = vk::RayTracingPipelineCreateInfoKHR()
                          .setStages(shaderStages)
                          .setGroups(shaderGroups)
                          .setMaxPipelineRayRecursionDepth(1)
                          .setLayout(m_pipelineLayout.handle());
    
    auto [result, pipeline] = m_device.handle().createRayTracingPipelineKHRUnique(
        vk::DeferredOperationKHR(), vk::PipelineCache(), info);
    
    if (result != vk::Result::eSuccess) {
        throw RayTracingPipelineCreationException{
            std::source_location::current()};
    }
    
    // Get shader group handle size
    const auto properties = m_device.physical_device().getProperties2<
        vk::PhysicalDeviceProperties2,
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    
    const auto rayTracingProperties = properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    const auto shaderGroupHandleSize = rayTracingProperties.shaderGroupHandleSize;
    
    RayTracingPipeline rayTracingPipeline(std::move(pipeline), std::move(m_pipelineLayout), m_device);
    rayTracingPipeline.m_shader_group_handle_size = shaderGroupHandleSize;
    
    return rayTracingPipeline;
}

} // namespace vw 