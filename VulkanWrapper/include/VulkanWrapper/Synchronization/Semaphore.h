#pragma once

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
    Semaphore build(Device &device) &&;
};
} // namespace vw
