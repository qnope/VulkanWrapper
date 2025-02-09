#include "Pipeline/PipelineLayout.h"

#include <VulkanWrapper/Pipeline/PipelineLayout.h>

vw::PipelineLayout *vw_create_pipeline_layout(const vw::Device *device) {
    return new vw::PipelineLayout(vw::PipelineLayoutBuilder(*device).build());
}

void vw_destroy_pipeline_layout(vw::PipelineLayout *pipelineLayout) {
    delete pipelineLayout;
}
