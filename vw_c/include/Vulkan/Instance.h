#pragma once

#include "../utils/array.h"

namespace vw {
class Instance;
}

vw::Instance *vw_create_instance(bool debugMode, ArrayConstString extensions);

void vw_destroy_instance(vw::Instance *instance);
