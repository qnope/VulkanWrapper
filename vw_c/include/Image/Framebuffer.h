#pragma once

#include <vulkan/vulkan.h>

namespace vw {
class Device;
class ImageView;
class RenderPass;
class Framebuffer;
} // namespace vw

extern "C" {
struct VwFramebufferCreateArguments {
    const vw::Device *device;
    const vw::RenderPass *render_pass;
    const vw::ImageView **image_views;
    int number_image_views;
    uint32_t width;
    uint32_t height;
};

vw::Framebuffer *
vw_create_framebuffer(const VwFramebufferCreateArguments *arguments);

uint32_t vw_framebuffer_width(const vw::Framebuffer *framebuffer);
uint32_t vw_framebuffer_height(const vw::Framebuffer *framebuffer);
VkFramebuffer vw_framebuffer_handle(const vw::Framebuffer *framebuffer);

void vw_destroy_framebuffer(vw::Framebuffer *framebuffer);
}
