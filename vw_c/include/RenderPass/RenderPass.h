#pragma once
#include "../Vulkan/Device.h"
#include "Subpass.h"

namespace vw {
class RenderPass;
}

extern "C" {
struct VwRenderPassCreateArguments {
    vw::Subpass **subpasses;
    int size;
};

vw::RenderPass *
vw_create_render_pass(const vw::Device *device,
                      const VwRenderPassCreateArguments *arguments);

VkRenderPass vw_render_pass_handle(const vw::RenderPass *render_pass);

void vw_destroy_render_pass(vw::RenderPass *renderPass);
}
