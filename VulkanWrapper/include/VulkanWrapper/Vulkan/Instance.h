#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
using InstanceCreationException = TaggedException<struct InstanceCreationTag>;

class Instance : public ObjectWithUniqueHandle<vk::UniqueInstance> {
    friend class InstanceBuilder;

  public:
    [[nodiscard]] DeviceFinder findGpu() const noexcept;

  private:
    Instance(vk::UniqueInstance instance, std::span<const char *> extensions,
             ApiVersion apiVersion) noexcept;

    std::vector<const char *> m_extensions;
    ApiVersion m_version;
};

class InstanceBuilder {
  public:
    InstanceBuilder &&addPortability() &&;
    InstanceBuilder &&addExtension(const char *extension) &&;
    InstanceBuilder &&addExtensions(std::span<const char *const> extensions) &&;
    InstanceBuilder &&setDebug() &&;
    InstanceBuilder &&setApiVersion(ApiVersion version) &&;

    Instance build() &&;

  private:
    vk::InstanceCreateFlags m_flags;
    std::vector<const char *> m_extensions;
    std::vector<const char *> m_layers;
    bool m_debug = true;
    ApiVersion m_version = ApiVersion::e10;
};

} // namespace vw
