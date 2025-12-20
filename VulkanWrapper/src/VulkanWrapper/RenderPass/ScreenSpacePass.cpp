#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"

#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

std::shared_ptr<const Pipeline> create_screen_space_pipeline(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const ShaderModule> vertex_shader,
    std::shared_ptr<const ShaderModule> fragment_shader,
    std::shared_ptr<const DescriptorSetLayout> descriptor_set_layout,
    vk::Format color_format, vk::Format depth_format,
    vk::CompareOp depth_compare_op,
    std::vector<vk::PushConstantRange> push_constants,
    std::optional<ColorBlendConfig> blend) {

    auto pipeline_layout_builder = PipelineLayoutBuilder(device);

    if (descriptor_set_layout) {
        std::move(pipeline_layout_builder)
            .with_descriptor_set_layout(descriptor_set_layout);
    }

    for (const auto &pc : push_constants) {
        std::move(pipeline_layout_builder).with_push_constant_range(pc);
    }

    auto pipeline_layout = std::move(pipeline_layout_builder).build();

    auto builder = GraphicsPipelineBuilder(device, std::move(pipeline_layout))
                       .add_shader(vk::ShaderStageFlagBits::eVertex,
                                   std::move(vertex_shader))
                       .add_shader(vk::ShaderStageFlagBits::eFragment,
                                   std::move(fragment_shader))
                       .with_dynamic_viewport_scissor()
                       .with_topology(vk::PrimitiveTopology::eTriangleStrip)
                       .with_cull_mode(vk::CullModeFlagBits::eNone)
                       .add_color_attachment(color_format, blend);

    if (depth_format != vk::Format::eUndefined) {
        std::move(builder)
            .set_depth_format(depth_format)
            .with_depth_test(false, depth_compare_op);
    }

    return std::move(builder).build();
}

} // namespace vw
