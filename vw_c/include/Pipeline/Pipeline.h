#pragma once

#include "../RenderPass/RenderPass.h"
#include "PipelineLayout.h"
#include "ShaderModule.h"

namespace vw {
class Pipeline;
}

extern "C" {
struct VwStageAndShader {
    VkShaderStageFlagBits stage;
    vw::ShaderModule *module;
};

struct VwGraphicsPipelineCreateArguments {
    const vw::Device *device;
    const vw::RenderPass *renderPass;

    const VwStageAndShader *stageAndShaders;
    int size;
    bool withViewport;
    bool withScissor;
    int widthViewport;
    int heightViewport;
    int widthScissor;
    int heightScissor;
    const vw::PipelineLayout *layout;
    int number_color_attachment;
};

vw::Pipeline *
vw_create_graphics_pipeline(const VwGraphicsPipelineCreateArguments *arguments);

VkPipeline vw_pipeline_handle(const vw::Pipeline *pipeline);

void vw_destroy_pipeline(vw::Pipeline *pipeline);
}
