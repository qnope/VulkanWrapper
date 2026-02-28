export module vw.descriptors:descriptor_set;
import std3rd;
import vulkan3rd;
import vw.utils;
import vw.vulkan;
import vw.sync;

export namespace vw {

class DescriptorSet : public ObjectWithHandle<vk::DescriptorSet> {
  public:
    DescriptorSet(vk::DescriptorSet handle,
                  std::vector<Barrier::ResourceState> resources) noexcept;

    const std::vector<Barrier::ResourceState> &resources() const noexcept;

  private:
    std::vector<Barrier::ResourceState> m_resources;
};

} // namespace vw
