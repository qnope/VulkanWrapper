#pragma once

#include "3DRenderer/3rd_party.h"
#include "3DRenderer/Core/Vulkan/ObjectWithHandle.h"
#include "3DRenderer/Core/fwd.h"

namespace r3d {
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

} // namespace r3d
