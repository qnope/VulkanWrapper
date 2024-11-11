#pragma once

#include "VulkanWrapper/Core/fwd.h"
#include "VulkanWrapper/Core/Vulkan/ObjectWithHandle.h"

namespace vw {
class Instance : public ObjectWithUniqueHandle<vk::UniqueInstance> {
    friend class InstanceBuilder;

  public:
    DeviceFinder findGpu() const noexcept;

  private:
    Instance(vk::UniqueInstance instance,
             std::vector<const char *> extensions) noexcept;

  private:
    std::vector<const char *> m_extensions;
};

class InstanceBuilder {
  public:
    InstanceBuilder &&addPortability() &&;
    InstanceBuilder &&addExtension(const char *extension) &&;
    InstanceBuilder &&addExtensions(std::vector<const char *> extensions) &&;
    InstanceBuilder &&setDebug() &&;

    Instance build() &&;

  private:
    vk::InstanceCreateFlags m_flags;
    std::vector<const char *> m_extensions;
    std::vector<const char *> m_layers;
    bool m_debug = true;
};

} // namespace vw
