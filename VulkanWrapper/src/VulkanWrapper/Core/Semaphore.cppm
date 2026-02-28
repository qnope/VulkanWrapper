export module vw.vulkan:semaphore;
import std3rd;
import vulkan3rd;
import vw.utils;
import :device;

export namespace vw {

class Semaphore : public ObjectWithUniqueHandle<vk::UniqueSemaphore> {
    friend class SemaphoreBuilder;

  public:
  private:
    using ObjectWithUniqueHandle<vk::UniqueSemaphore>::ObjectWithUniqueHandle;
};

class SemaphoreBuilder {
  public:
    SemaphoreBuilder(std::shared_ptr<const Device> device);
    Semaphore build();

  private:
    std::shared_ptr<const Device> m_device;
};

} // namespace vw
