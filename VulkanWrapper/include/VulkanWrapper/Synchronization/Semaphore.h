#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

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
