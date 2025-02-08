#pragma once

#include "../utils/array.h"

namespace vw {
class Instance;
class DeviceFinder;
} // namespace vw

vw::Instance *vw_create_instance(bool debugMode,
                                 vw_ArrayConstString extensions);

vw::DeviceFinder *vw_find_gpu_from_instance(const vw::Instance *instance);

void vw_destroy_device_finder(vw::DeviceFinder *deviceFinder);

void vw_destroy_instance(vw::Instance *instance);
