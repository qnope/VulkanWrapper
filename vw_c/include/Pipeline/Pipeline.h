#pragma once

#include "../RenderPass/RenderPass.h"
#include "PipelineLayout.h"
#include "ShaderModule.h"

namespace vw {
class Pipeline;
}

extern "C" {
struct vw_StageAndShader {
    VkShaderStageFlagBits stage;
    vw::ShaderModule *module;
};

struct vw_GraphicsPipelineArguments {
    const vw::Device *device;
    const vw::RenderPass *renderPass;

    const vw_StageAndShader *stageAndShaders;
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
vw_create_graphics_pipeline(vw_GraphicsPipelineArguments arguments);

void vw_destroy_pipeline(vw::Pipeline *pipeline);
}
