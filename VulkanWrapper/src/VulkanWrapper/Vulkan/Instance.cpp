#include "VulkanWrapper/Vulkan/Instance.h"

#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include "VulkanWrapper/Vulkan/PhysicalDevice.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace vw {

const vk::detail::DispatchLoaderDynamic &DefaultDispatcher() {
    return VULKAN_HPP_DEFAULT_DISPATCHER;
}

namespace {

vk::Bool32
debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
              vk::DebugUtilsMessageTypeFlagsEXT type,
              const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void * /*pUserData*/) {

    // Throw exception immediately for validation errors
    if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        throw ValidationLayerException(severity, type, pCallbackData->pMessage);
    }

    // Print warnings to stderr
    if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        std::cerr << "[VALIDATION WARNING] " << pCallbackData->pMessage
                  << std::endl;
    }

    return vk::False;
}

} // namespace

Instance::Impl::Impl(vk::UniqueInstance inst,
                     vk::UniqueDebugUtilsMessengerEXT debugMsgr,
                     std::span<const char *> exts,
                     ApiVersion apiVersion) noexcept
    : instance{std::move(inst)}
    , debugMessenger{std::move(debugMsgr)}
    , extensions{exts.begin(), exts.end()}
    , version{apiVersion} {}

Instance::Instance(vk::UniqueInstance instance,
                   vk::UniqueDebugUtilsMessengerEXT debugMessenger,
                   std::span<const char *> extensions,
                   ApiVersion apiVersion) noexcept
    : m_impl{std::make_shared<Impl>(std::move(instance),
                                    std::move(debugMessenger), extensions,
                                    apiVersion)} {}

vk::Instance Instance::handle() const noexcept { return *m_impl->instance; }

DeviceFinder Instance::findGpu() const noexcept {
    auto supportVersion = [this](const PhysicalDevice &device) {
        return device.api_version() >= m_impl->version;
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
    m_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return std::move(*this);
}

InstanceBuilder &&InstanceBuilder::setApiVersion(ApiVersion version) && {
    m_version = version;
    return std::move(*this);
}

std::shared_ptr<Instance> InstanceBuilder::build() && {
    const std::vector<const char *> layers = {"VK_LAYER_KHRONOS_validation"};

    auto appInfo = vk::ApplicationInfo()
                       .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
                       .setApiVersion(m_version)
                       .setPEngineName("3D Renderer")
                       .setEngineVersion(VK_MAKE_VERSION(1, 0, 0));

    // Add debug utils extension if debug mode is enabled
    if (m_debug) {
    }

    vk::InstanceCreateInfo info;
    info.setFlags(m_flags)
        .setPEnabledExtensionNames(m_extensions)
        .setPApplicationInfo(&appInfo);

    if (m_debug) {
        info.setPEnabledLayerNames(layers);
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    auto instance = check_vk(vk::createInstanceUnique(info),
                             "Failed to create Vulkan instance");

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    // Create debug messenger if debug mode is enabled
    vk::UniqueDebugUtilsMessengerEXT debugMessenger;
    if (m_debug) {
        auto messengerInfo =
            vk::DebugUtilsMessengerCreateInfoEXT()
                .setMessageSeverity(
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
                .setMessageType(
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
                .setPfnUserCallback(debugCallback);

        debugMessenger = check_vk(
            instance->createDebugUtilsMessengerEXTUnique(messengerInfo),
            "Failed to create debug messenger");
    }

    return std::shared_ptr<Instance>(new Instance(std::move(instance),
                                                  std::move(debugMessenger),
                                                  m_extensions, m_version));
}

} // namespace vw
