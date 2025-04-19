#include "VulkanWrapper/Vulkan/Instance.h"

#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include "VulkanWrapper/Vulkan/PhysicalDevice.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace vw {

const vk::detail::DispatchLoaderDynamic &DefaultDispatcher() {
    return VULKAN_HPP_DEFAULT_DISPATCHER;
}

Instance::Instance(vk::UniqueInstance instance,
                   std::span<const char *> extensions,
                   ApiVersion apiVersion) noexcept
    : ObjectWithUniqueHandle<vk::UniqueInstance>{std::move(instance)}
    , m_extensions{extensions.begin(), extensions.end()}
    , m_version{apiVersion} {}

DeviceFinder Instance::findGpu() const noexcept {
    auto supportVersion = [this](const PhysicalDevice &device) {
        return device.api_version() >= m_version;
    };
    auto physicalDevices = [&] {
        auto devices = handle().enumeratePhysicalDevices().value;
        auto as_vector =
            std::vector<PhysicalDevice>(devices.begin(), devices.end());

        for (const auto &device : as_vector)
            std::cout << "Found: " << device.name() << std::endl;

        return as_vector | std::views::filter(supportVersion) | to<std::vector>;
    }();

    return DeviceFinder{physicalDevices};
}

InstanceBuilder &&InstanceBuilder::addPortability() && {
    // m_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    // m_flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
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

InstanceBuilder &&InstanceBuilder::setApiVersion(ApiVersion version) && {
    m_version = version;
    return std::move(*this);
}

Instance InstanceBuilder::build() && {
    const std::vector<const char *> layers = {"VK_LAYER_KHRONOS_validation"};

    auto appInfo = vk::ApplicationInfo()
                       .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
                       .setApiVersion(m_version)
                       .setPEngineName("3D Renderer")
                       .setEngineVersion(VK_MAKE_VERSION(1, 0, 0));

    vk::InstanceCreateInfo info;
    info.setFlags(m_flags)
        .setPEnabledExtensionNames(m_extensions)
        .setPEnabledLayerNames(layers)
        .setPApplicationInfo(&appInfo);

    if (m_debug) {
        info.setPEnabledLayerNames(layers);
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    auto [result, instance] = vk::createInstanceUnique(info);

    if (result != vk::Result::eSuccess) {
        std::cout << unsigned(result) << std::endl;
        throw InstanceCreationException{std::source_location::current()};
        system("pause");
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    return {std::move(instance), m_extensions, m_version};
}

} // namespace vw
