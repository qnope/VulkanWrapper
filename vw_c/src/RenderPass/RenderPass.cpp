#include "RenderPass/RenderPass.h"

#include <VulkanWrapper/RenderPass/RenderPass.h>
#include <VulkanWrapper/RenderPass/Subpass.h>

vw::RenderPass *
vw_create_render_pass(const vw::Device *device,
                      const VwRenderPassCreateArguments *arguments) {
    vw::RenderPassBuilder builder(*device);
    for (int i = 0; i < arguments->size; ++i) {
        std::move(builder).add_subpass(std::move(*arguments->subpasses[i]));
    }
    return new vw::RenderPass(std::move(builder).build());
}

VkRenderPass vw_render_pass_handle(const vw::RenderPass *render_pass) {
    return render_pass->handle();
}

void vw_destroy_render_pass(vw::RenderPass *renderPass) { delete renderPass; }
