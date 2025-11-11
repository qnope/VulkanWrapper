#include "VulkanWrapper/Pipeline/RayTracingPipeline.h"

#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

RayTracingPipeline::RayTracingPipeline(
    vk::UniquePipeline pipeline, PipelineLayout pipeline_layout,
    ShaderBindingTableBuffer shader_binding_table_buffer_gen,
    ShaderBindingTableBuffer shader_binding_table_buffer_miss,
    ShaderBindingTableBuffer shader_binding_table_buffer_hit,
    ShaderBindingTable shader_binding_table) noexcept
    : ObjectWithUniqueHandle<vk::UniquePipeline>(std::move(pipeline))
    , m_layout(std::move(pipeline_layout))
    , m_shader_binding_table_buffer_ray_gen{std::move(
          shader_binding_table_buffer_gen)}
    , m_shader_binding_table_buffer_miss{std::move(
          shader_binding_table_buffer_miss)}
    , m_shader_binding_table_buffer_hit{std::move(
          shader_binding_table_buffer_hit)}
    , m_shader_binding_table{std::move(shader_binding_table)} {}

const PipelineLayout &RayTracingPipeline::layout() const noexcept {
    return m_layout;
}

ShaderBindingTable
RayTracingPipeline::get_shader_binding_table() const noexcept {
    return m_shader_binding_table;
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

    auto [shader_binding_table, buffer_gen, buffer_miss, buffer_hit] =
        create_shader_binding_table(*pipeline);

    RayTracingPipeline rayTracingPipeline(
        std::move(pipeline), std::move(m_pipelineLayout), std::move(buffer_gen),
        std::move(buffer_miss), std::move(buffer_hit),
        std::move(shader_binding_table));

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

namespace {
const auto align_up(int size, int alignment) {
    return (size + (alignment - 1)) & (~(alignment - 1));
}

void fill_table(ShaderBindingTable &table, ShaderBindingTableBuffer &buffer_gen,
                ShaderBindingTableBuffer &buffer_miss,
                ShaderBindingTableBuffer &buffer_hit,
                const std::vector<std::byte> &handles, uint32_t handle_size,
                uint32_t closest_hit_count, uint32_t miss_count) {
    std::vector<std::byte> data_gen(buffer_gen.size_bytes());
    std::vector<std::byte> data_miss(buffer_gen.size_bytes());
    std::vector<std::byte> data_hit(buffer_gen.size_bytes());
    std::copy(handles.begin(), handles.begin() + buffer_gen.size_bytes(),
              data_gen.begin());
    std::copy(handles.begin() + 1 * buffer_gen.size_bytes(),
              handles.begin() + 2 * buffer_gen.size_bytes(), data_miss.begin());
    std::copy(handles.begin() + 2 * buffer_gen.size_bytes(),
              handles.begin() + 3 * buffer_gen.size_bytes(), data_hit.begin());

    buffer_gen.copy(data_gen, 0);
    buffer_miss.copy(data_miss, 0);
    buffer_hit.copy(data_hit, 0);
}

} // namespace

std::tuple<ShaderBindingTable, ShaderBindingTableBuffer,
           ShaderBindingTableBuffer, ShaderBindingTableBuffer>
RayTracingPipelineBuilder::create_shader_binding_table(
    vk::Pipeline pipeline) const {
    const auto ray_generation_count = 1;
    const auto closest_hit_count = m_closest_hit_shaders.size();
    const auto miss_count = m_miss_shaders.size();

    const auto handle_count = 1 + closest_hit_count + miss_count;

    const auto rt_properties =
        m_device.physical_device()
            .getProperties2<vk::PhysicalDeviceProperties2,
                            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>()
            .get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    const uint32_t handleSizeAligned =
        align_up(rt_properties.shaderGroupHandleSize,
                 rt_properties.shaderGroupHandleAlignment);

    const std::vector<std::byte> handles =
        m_device.handle()
            .getRayTracingShaderGroupHandlesKHR<std::byte>(
                pipeline, 0, handle_count, handleSizeAligned * handle_count)
            .value;

    vk::StridedDeviceAddressRegionKHR generation_region;
    vk::StridedDeviceAddressRegionKHR closest_hit_region;
    vk::StridedDeviceAddressRegionKHR miss_region;

    generation_region.stride = handleSizeAligned;
    generation_region.size = handleSizeAligned;

    closest_hit_region.stride = handleSizeAligned;
    closest_hit_region.size = handleSizeAligned;

    miss_region.stride = handleSizeAligned;
    miss_region.size = handleSizeAligned;

    const auto buffer_size = handleSizeAligned;

    auto buffer_gen =
        m_allocator.create_buffer<ShaderBindingTableBuffer>(buffer_size);
    auto buffer_miss =
        m_allocator.create_buffer<ShaderBindingTableBuffer>(buffer_size);
    auto buffer_hit =
        m_allocator.create_buffer<ShaderBindingTableBuffer>(buffer_size);

    generation_region.deviceAddress = buffer_gen.device_address();
    closest_hit_region.deviceAddress = buffer_hit.device_address();
    miss_region.deviceAddress = buffer_miss.device_address();

    ShaderBindingTable table{generation_region, miss_region,
                             closest_hit_region};

    fill_table(table, buffer_gen, buffer_miss, buffer_hit, handles,
               rt_properties.shaderGroupHandleSize, closest_hit_count,
               miss_count);

    return {table, std::move(buffer_gen), std::move(buffer_miss),
            std::move(buffer_hit)};
}

} // namespace vw
