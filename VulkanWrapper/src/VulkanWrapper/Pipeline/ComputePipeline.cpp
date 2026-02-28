module vw.pipeline;
import std3rd;
import vulkan3rd;
import vw.utils;
import vw.vulkan;
import vw.descriptors;

namespace vw {

ComputePipelineBuilder::ComputePipelineBuilder(
    std::shared_ptr<const Device> device, PipelineLayout pipelineLayout)
    : m_device{std::move(device)}
    , m_pipelineLayout{std::move(pipelineLayout)} {}

ComputePipelineBuilder &
ComputePipelineBuilder::set_shader(std::shared_ptr<const ShaderModule> module) {
    m_shaderModule = std::move(module);
    return *this;
}

std::shared_ptr<const Pipeline> ComputePipelineBuilder::build() {
    auto stageInfo = vk::PipelineShaderStageCreateInfo()
                         .setStage(vk::ShaderStageFlagBits::eCompute)
                         .setModule(m_shaderModule->handle())
                         .setPName("main");

    auto createInfo =
        vk::ComputePipelineCreateInfo().setStage(stageInfo).setLayout(
            m_pipelineLayout.handle());

    auto pipeline = check_vk(m_device->handle().createComputePipelineUnique(
                                 vk::PipelineCache(), createInfo),
                             "Failed to create compute pipeline");

    return std::make_shared<Pipeline>(std::move(pipeline),
                                      std::move(m_pipelineLayout));
}

} // namespace vw
