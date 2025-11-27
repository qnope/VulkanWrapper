#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

using SemaphoreCreationException = TaggedException<struct SemaphoreCreationTag>;

class Semaphore : public ObjectWithUniqueHandle<vk::UniqueSemaphore> {
    friend class SemaphoreBuilder;

  public:
  private:
    using ObjectWithUniqueHandle<vk::UniqueSemaphore>::ObjectWithUniqueHandle;
};

class SemaphoreBuilder {
  public:
    SemaphoreBuilder(const Device &device);
    Semaphore build() &&;

  private:
    const Device *m_device;
};
} // namespace vw
