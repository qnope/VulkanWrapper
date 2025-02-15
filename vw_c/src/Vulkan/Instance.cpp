#include "Vulkan/Instance.h"

#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Vulkan/Instance.h>

vw::Instance *vw_create_instance(const VwInstanceCreateArguments *arguments) {
    std::span extensions_as_span(arguments->extensions,
                                 arguments->extensions_count);

    if (arguments->debug_mode)
        return new vw::Instance{vw::InstanceBuilder()
                                    .addExtensions(extensions_as_span)
                                    .addPortability()
                                    .setDebug()
                                    .build()};
    return new vw::Instance{vw::InstanceBuilder()
                                .addExtensions(extensions_as_span)
                                .addPortability()
                                .build()};
}

vw::DeviceFinder *vw_find_gpu_from_instance(const vw::Instance *instance) {
    return new vw::DeviceFinder{instance->findGpu()};
}

void vw_destroy_instance(vw::Instance *instance) { delete instance; }

void vw_destroy_device_finder(vw::DeviceFinder *deviceFinder) {
    delete deviceFinder;
}
