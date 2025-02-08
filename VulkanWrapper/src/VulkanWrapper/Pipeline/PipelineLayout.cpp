#include "VulkanWrapper/Pipeline/PipelineLayout.h"

#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
PipelineLayoutBuilder::PipelineLayoutBuilder(const Device &device)
    : m_device{device} {}

PipelineLayout PipelineLayoutBuilder::build() && {
    const auto info = vk::PipelineLayoutCreateInfo();

    auto [result, layout] = m_device.handle().createPipelineLayoutUnique(info);

    if (result != vk::Result::eSuccess)
        throw PipelineLayoutCreationException{std::source_location::current()};

    return PipelineLayout(std::move(layout));
}
} // namespace vw
