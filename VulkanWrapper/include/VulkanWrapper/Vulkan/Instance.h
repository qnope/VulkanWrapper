#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
using InstanceCreationException = TaggedException<struct InstanceCreationTag>;

class Instance : public ObjectWithUniqueHandle<vk::UniqueInstance> {
    friend class InstanceBuilder;

  public:
    DeviceFinder findGpu() const noexcept;

  private:
    Instance(vk::UniqueInstance instance,
             std::span<const char *> extensions) noexcept;

  private:
    std::vector<const char *> m_extensions;
};

class InstanceBuilder {
  public:
    InstanceBuilder &&addPortability() &&;
    InstanceBuilder &&addExtension(const char *extension) &&;
    InstanceBuilder &&addExtensions(std::span<const char * const> extensions) &&;
    InstanceBuilder &&setDebug() &&;

    Instance build() &&;

  private:
    vk::InstanceCreateFlags m_flags;
    std::vector<const char *> m_extensions;
    std::vector<const char *> m_layers;
    bool m_debug = true;
};

} // namespace vw
