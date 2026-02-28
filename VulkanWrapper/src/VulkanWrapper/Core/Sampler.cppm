export module vw.vulkan:sampler;
import std3rd;
import vulkan3rd;
import vw.utils;
import :device;

export namespace vw {

class Sampler : public ObjectWithUniqueHandle<vk::UniqueSampler> {
  public:
    using ObjectWithUniqueHandle<vk::UniqueSampler>::ObjectWithHandle;

  private:
};

class SamplerBuilder {
  public:
    SamplerBuilder(std::shared_ptr<const Device> device);

    std::shared_ptr<const Sampler> build();

  private:
    std::shared_ptr<const Device> m_device;
    vk::SamplerCreateInfo m_info;
};

} // namespace vw
