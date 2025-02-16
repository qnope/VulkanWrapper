#pragma once

#include "../utils/utils.h"

namespace vw {
class Instance;
class DeviceFinder;
} // namespace vw

extern "C" {
struct VwInstanceCreateArguments {
    VwStringArray extensions;
    bool debug_mode;
};

vw::Instance *vw_create_instance(const VwInstanceCreateArguments *arguments);

vw::DeviceFinder *vw_find_gpu_from_instance(const vw::Instance *instance);

void vw_destroy_device_finder(vw::DeviceFinder *deviceFinder);

void vw_destroy_instance(vw::Instance *instance);
}
