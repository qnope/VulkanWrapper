#pragma once

#include "../Vulkan/Device.h"

namespace vw {
class PipelineLayout;
}

extern "C" {

vw::PipelineLayout *vw_create_pipeline_layout(const vw::Device *device);
void vw_destroy_pipeline_layout(vw::PipelineLayout *pipelineLayout);
}
