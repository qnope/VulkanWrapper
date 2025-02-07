#include "Vulkan/Instance.h"

#include <VulkanWrapper/Vulkan/Instance.h>

vw::Instance *vw_create_instance(bool debugMode, ArrayConstString extensions) {
    std::span extensions_as_span(extensions.array, extensions.size);

    if (debugMode)
        return new vw::Instance{vw::InstanceBuilder()
                                    .addExtensions(extensions_as_span)
                                    .setDebug()
                                    .build()};
    return new vw::Instance{vw::InstanceBuilder()
                                .addExtensions(extensions_as_span)
                                .addPortability()
                                .build()};
}

void vw_destroy_instance(vw::Instance *instance) { delete instance; }
