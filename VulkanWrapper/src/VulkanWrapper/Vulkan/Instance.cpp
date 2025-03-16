#include "VulkanWrapper/Vulkan/Instance.h"

#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include "VulkanWrapper/Vulkan/PhysicalDevice.h"

namespace vw {

Instance::Instance(vk::UniqueInstance instance,
                   std::span<const char *> extensions) noexcept
    : ObjectWithUniqueHandle<vk::UniqueInstance>{std::move(instance)}
    , m_extensions{extensions.begin(), extensions.end()} {}

DeviceFinder Instance::findGpu() const noexcept {
    auto physicalDevices = [&] {
        auto devices = handle().enumeratePhysicalDevices().value;
        return std::vector<PhysicalDevice>(devices.begin(), devices.end());
    }();
    return DeviceFinder{physicalDevices};
}

InstanceBuilder &&InstanceBuilder::addPortability() && {
    m_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    m_flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    return std::move(*this);
}

InstanceBuilder &&InstanceBuilder::addExtension(const char *extension) && {
    m_extensions.push_back(extension);
    return std::move(*this);
}

InstanceBuilder &&
InstanceBuilder::addExtensions(std::span<char const *const> extensions) && {
    for (const auto *extension : extensions) {
        m_extensions.push_back(extension);
    }
    return std::move(*this);
}

InstanceBuilder &&InstanceBuilder::setDebug() && {
    m_debug = true;
    return std::move(*this);
}

Instance InstanceBuilder::build() && {
    const std::vector<const char *> layers = {"VK_LAYER_KHRONOS_validation"};

    auto appInfo = vk::ApplicationInfo()
                       .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
                       .setApiVersion(vk::ApiVersion12)
                       .setPEngineName("3D Renderer")
                       .setEngineVersion(VK_MAKE_VERSION(1, 0, 0));

    vk::InstanceCreateInfo info;
    info.setFlags(m_flags)
        .setPEnabledExtensionNames(m_extensions)
        .setPEnabledLayerNames(layers);
    info.setPApplicationInfo(&appInfo);

    if (m_debug) {
        info.setPEnabledLayerNames(layers);
    }

    auto [result, instance] = vk::createInstanceUnique(info);

    if (result != vk::Result::eSuccess) {
        throw InstanceCreationException{std::source_location::current()};
    }

    return {std::move(instance), m_extensions};
}

} // namespace vw
