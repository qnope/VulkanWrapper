#include "VulkanWrapper/Pipeline/PipelineLayout.h"

#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
PipelineLayout PipelineLayoutBuilder::build(Device &device) && {
    const auto info = vk::PipelineLayoutCreateInfo();

    auto [result, layout] = device.handle().createPipelineLayoutUnique(info);

    if (result != vk::Result::eSuccess)
        throw PipelineLayoutCreationException{std::source_location::current()};

    return PipelineLayout(std::move(layout));
}
} // namespace vw
