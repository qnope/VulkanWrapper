#include "RenderPass/RenderPass.h"

#include <VulkanWrapper/RenderPass/RenderPass.h>
#include <VulkanWrapper/RenderPass/Subpass.h>

vw::RenderPass *vw_create_render_pass(const vw::Device *device,
                                      vw::Subpass **subpasses, int size) {
    vw::RenderPassBuilder builder(*device);
    for (int i = 0; i < size; ++i) {
        std::move(builder).add_subpass(std::move(*subpasses[i]));
    }
    return new vw::RenderPass(std::move(builder).build());
}

void vw_destroy_render_pass(vw::RenderPass *renderPass) { delete renderPass; }
