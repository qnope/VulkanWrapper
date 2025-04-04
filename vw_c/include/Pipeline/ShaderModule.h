#pragma once

#include "../utils/utils.h"

namespace vw {
class ShaderModule;
class Device;
} // namespace vw

extern "C" {
vw::ShaderModule *
vw_create_shader_module_from_spirv_file(const vw::Device *device,
                                        VwString path);

void vw_destroy_shader_module(vw::ShaderModule *shaderModule);
}
