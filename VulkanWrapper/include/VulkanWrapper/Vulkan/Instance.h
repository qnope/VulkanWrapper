#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include <memory>

namespace vw {
using InstanceCreationException = TaggedException<struct InstanceCreationTag>;

class Instance {
    friend class InstanceBuilder;

  public:
    Instance(const Instance &) = delete;
    Instance(Instance &&) = delete;

    Instance &operator=(Instance &&) = delete;
    Instance &operator=(const Instance &) = delete;

    [[nodiscard]] vk::Instance handle() const noexcept;

    [[nodiscard]] DeviceFinder findGpu() const noexcept;

  private:
    Instance(vk::UniqueInstance instance, std::span<const char *> extensions,
             ApiVersion apiVersion) noexcept;

    struct Impl {
        vk::UniqueInstance instance;
        std::vector<const char *> extensions;
        ApiVersion version;

        Impl(vk::UniqueInstance inst, std::span<const char *> exts,
             ApiVersion apiVersion) noexcept;

        Impl(const Impl &) = delete;
        Impl &operator=(const Impl &) = delete;
        Impl(Impl &&) = delete;
        Impl &operator=(Impl &&) = delete;
    };

    std::shared_ptr<Impl> m_impl;
};

class InstanceBuilder {
  public:
    InstanceBuilder &&addPortability() &&;
    InstanceBuilder &&addExtension(const char *extension) &&;
    InstanceBuilder &&addExtensions(std::span<const char *const> extensions) &&;
    InstanceBuilder &&setDebug() &&;
    InstanceBuilder &&setApiVersion(ApiVersion version) &&;

    std::shared_ptr<Instance> build() &&;

  private:
    vk::InstanceCreateFlags m_flags;
    std::vector<const char *> m_extensions;
    std::vector<const char *> m_layers;
    bool m_debug = true;
    ApiVersion m_version = ApiVersion::e10;
};

} // namespace vw
