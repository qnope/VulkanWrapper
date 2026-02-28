export module vw.pipeline:compute_pipeline;
import std3rd;
import vulkan3rd;
import vw.vulkan;
import :pipeline;

export namespace vw {

class ShaderModule;

class ComputePipelineBuilder {
  public:
    ComputePipelineBuilder(std::shared_ptr<const Device> device,
                           PipelineLayout pipelineLayout);

    ComputePipelineBuilder &
    set_shader(std::shared_ptr<const ShaderModule> module);

    std::shared_ptr<const Pipeline> build();

  private:
    std::shared_ptr<const Device> m_device;
    PipelineLayout m_pipelineLayout;
    std::shared_ptr<const ShaderModule> m_shaderModule;
};
} // namespace vw
