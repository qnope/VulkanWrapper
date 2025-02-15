#include "Pipeline/Pipeline.h"

#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Pipeline/ShaderModule.h>

vw::Pipeline *
vw_create_graphics_pipeline(const VwGraphicsPipelineCreateArguments *args) {
    auto builder =
        vw::GraphicsPipelineBuilder(*args->device, *args->renderPass);

    if (args->withScissor)
        std::move(builder).with_fixed_scissor(args->widthScissor,
                                              args->heightScissor);

    if (args->withViewport)
        std::move(builder).with_fixed_viewport(args->widthViewport,
                                               args->heightViewport);

    for (int i = 0; i < args->size; ++i) {
        std::move(builder).add_shader(
            vk::ShaderStageFlagBits{args->stageAndShaders[i].stage},
            std::move(*args->stageAndShaders[i].module));
    }

    for (int i = 0; i < args->number_color_attachment; ++i) {
        std::move(builder).add_color_attachment();
    }

    if (args->layout)
        std::move(builder).with_pipeline_layout(*args->layout);

    return new vw::Pipeline{std::move(builder).build()};
}

VkPipeline vw_pipeline_handle(const vw::Pipeline *pipeline) {
    return pipeline->handle();
}

void vw_destroy_pipeline(vw::Pipeline *pipeline) { delete pipeline; }
