#include "Pipeline/ShaderModule.h"

#include <VulkanWrapper/Pipeline/ShaderModule.h>

vw::ShaderModule *
vw_create_shader_module_from_spirv_file(const vw::Device *device,
                                        const char *path) {
    return new vw::ShaderModule(
        vw::ShaderModule::create_from_spirv_file(*device, path));
}

void vw_destroy_shader_module(vw::ShaderModule *shaderModule) {
    delete shaderModule;
}
