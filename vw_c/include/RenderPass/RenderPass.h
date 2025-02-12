#pragma once
#include "../Vulkan/Device.h"
#include "Subpass.h"

namespace vw {
class RenderPass;
}

extern "C" {
vw::RenderPass *vw_create_render_pass(const vw::Device *device,
                                      vw::Subpass **subpasses, int size);

VkRenderPass vw_render_pass_handle(const vw::RenderPass *render_pass);

void vw_destroy_render_pass(vw::RenderPass *renderPass);
}
